/*
** Copyright 2020 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#include <amqpprox_connector.h>

#include <amqpprox_buffer.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_closeerror.h>
#include <amqpprox_connectorutil.h>
#include <amqpprox_constants.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_fieldvalue.h>
#include <amqpprox_flowtype.h>
#include <amqpprox_frame.h>
#include <amqpprox_logging.h>
#include <amqpprox_reply.h>
#include <amqpprox_sessionstate.h>

#include <iostream>
#include <string_view>

namespace Bloomberg {
namespace amqpprox {

namespace {

std::ostream &streamOutMethod(std::ostream &os, const Method &method)
{
    switch (method.methodType) {
    case methods::Close::methodType():
        return os << "Close";
    case methods::CloseOk::methodType():
        return os << "CloseOk";
    case methods::Open::methodType():
        return os << "Open";
    case methods::OpenOk::methodType():
        return os << "OpenOk";
    case methods::Start::methodType():
        return os << "Start";
    case methods::StartOk::methodType():
        return os << "StartOk";
    case methods::Tune::methodType():
        return os << "Tune";
    case methods::TuneOk::methodType():
        return os << "TuneOk";
    }
    return os << "Unknown: " << method.methodType;
}

template <typename T>
void decodeMethod(T            *t,
                  const Method &method,
                  Buffer       &buffer,
                  FlowType      direction)
{
    if (method.methodType != T::methodType()) {
        std::ostringstream oss;
        oss << "Expected " << typeid(T).name() << ", got: ";
        streamOutMethod(oss, method);
        if (method.methodType == methods::Close::methodType() &&
            method.classType == methods::Close::classType() &&
            direction == FlowType::EGRESS) {
            methods::Close closeMethod;
            if (!methods::Close::decode(&closeMethod, buffer)) {
                oss << ". And Failed to decode received close method from "
                       "server";
                throw std::runtime_error(oss.str());
            }
            else {
                throw CloseError(oss.str(), closeMethod);
            }
        }
        else {
            throw std::runtime_error(oss.str());
        }
    }

    if (!T::decode(t, buffer)) {
        std::ostringstream oss;
        oss << "Failed to decode " << typeid(T).name();
        throw std::runtime_error(oss.str());
    }
}

const Buffer protocolHeader(Constants::protocolHeader(),
                            Constants::protocolHeaderLength());
const Buffer legacyProtocolHeader(Constants::legacyProtocolHeader(),
                                  Constants::legacyProtocolHeaderLength());

}

Connector::Connector(SessionState    *sessionState,
                     EventSource     *eventSource,
                     BufferPool      *bufferPool,
                     std::string_view localHostname)
: d_state(State::AWAITING_PROTOCOL_HEADER)
, d_synthesizedStart(ConnectorUtil::synthesizedStart())
, d_synthesizedTune(ConnectorUtil::synthesizedTune())
, d_sessionState_p(sessionState)
, d_eventSource_p(eventSource)
, d_bufferPool_p(bufferPool)
, d_synthesizedReplyBuffer()
, d_buffer()
, d_sendToIngressSide(false)
, d_reconnection(false)
, d_localHostname(localHostname)
{
}

void Connector::receive(const Buffer &buffer)
{
    if (d_state == State::AWAITING_PROTOCOL_HEADER) {
        if (buffer.equalContents(protocolHeader) ||
            buffer.equalContents(legacyProtocolHeader)) {
            sendResponse(d_synthesizedStart, true);
            d_state = State::START_SENT;
        }
        else {
            LOG_WARN << "Incorrect header passed. " << buffer.size()
                     << "bytes";
            d_buffer            = protocolHeader;
            d_sendToIngressSide = true;
            d_state             = State::ERROR;
        }
    }
    else {
        d_state = State::ERROR;
    }
}

void Connector::receive(const Method &method, FlowType direction)
{
    d_buffer = Buffer();
    Buffer methodPayload(method.payload, method.length);

    // Normal pass-through: just look for CloseOk
    switch (d_state) {
    case State::OPEN: {
        // Once the session is fully open and passing through messages, we can
        // discard the synthesized reply buffers
        d_synthesizedReplyBuffer.release();

        if (method.methodType == methods::CloseOk::methodType() ||
            method.methodType == methods::Close::methodType()) {
            // We catch both Close and CloseOk here because of catching one
            // side intentionally closing the connection, and the client
            // misbehaving and not respecting it with a CloseOk. This should
            // still count as graceful.
            LOG_TRACE << "Close/CloseOk";
            d_state = State::CLOSED;
        }
    } break;
    // Acting as a server
    case State::START_SENT: {
        decodeMethod(&d_startOk, method, methodPayload, direction);

        LOG_TRACE << "StartOk: " << d_startOk;

        sendResponse(d_synthesizedTune, true);
        d_state = State::TUNE_SENT;
    } break;
    case State::TUNE_SENT: {
        decodeMethod(&d_tuneOk, method, methodPayload, direction);

        LOG_TRACE << "TuneOk: " << d_tuneOk;

        d_state = State::AWAITING_OPEN;
    } break;

    case State::AWAITING_OPEN: {
        decodeMethod(&d_open, method, methodPayload, direction);

        d_sessionState_p->setVirtualHost(d_open.virtualHost());
        d_eventSource_p->connectionVhostEstablished().emit(
            d_sessionState_p->id(), d_open.virtualHost());

        LOG_TRACE << "Open: " << d_open;

        d_state = State::AWAITING_CONNECTION;
        d_connectionCreationHandler();
    } break;
    // Acting as a client
    case State::AWAITING_CONNECTION: {
        decodeMethod(&d_receivedStart, method, methodPayload, direction);

        LOG_TRACE << "Server Start: " << d_receivedStart;

        auto clientEndpoint    = d_sessionState_p->getIngress().second;
        auto inboundListenPort = d_sessionState_p->getIngress().first.port();
        auto outboundLocalPort = d_sessionState_p->getEgress().first.port();

        ConnectorUtil::injectProxyClientIdent(
            &d_startOk,
            d_sessionState_p->hostname(clientEndpoint),
            clientEndpoint.port(),
            d_localHostname,
            inboundListenPort,
            outboundLocalPort,
            d_sessionState_p->getIngressSecured());

        sendResponse(d_startOk, false);
        d_state = State::STARTOK_SENT;
    } break;
    // Acting as a client
    case State::STARTOK_SENT: {
        decodeMethod(&d_receivedTune, method, methodPayload, direction);
        LOG_TRACE << "Server Tune: " << d_receivedTune;

        sendResponse(d_tuneOk, false);

        methods::Open openCopy = d_open;
        openCopy.setVirtualHost(d_sessionState_p->getBackendVirtualHost());
        sendResponse(openCopy, false);

        d_state = State::OPEN_SENT;
    } break;
    case State::OPEN_SENT: {
        if (method.methodType == methods::OpenOk::methodType()) {
            // We are good to go
            d_state = State::OPEN;

            d_eventSource_p->connectionEstablished().emit(
                d_sessionState_p->id());

            if (!d_reconnection) {
                sendResponse(d_openOk, true);
            }

            if (d_connectionReadyHandler) {
                d_connectionReadyHandler();
            }
        }
    } break;
    case State::CLIENT_CLOSE_SENT: {
        if (direction == FlowType::INGRESS &&
            method.methodType == methods::CloseOk::methodType()) {
            LOG_TRACE << "Received CloseOk confirmation from client. Will now "
                         "initiate Close with server.";
            // happy path, received CloseOk from client after sending them
            // Close
            synthesizeClose(false);  // sending close to server
        }
        else if (method.methodType == methods::Close::methodType()) {
            if (direction == FlowType::INGRESS) {
                LOG_INFO << "Received Close confirmation from client while "
                            "waiting for CloseOk. Will now initiate Close "
                            "with server.";
                sendResponse(d_closeOk, true);
                synthesizeClose(false);  // sending close to server
            }
            else {
                // no need to send Close to server if received Close from
                // server
                LOG_WARN << "Received unexpected Close from server before "
                            "sending Close to server.";
                sendResponse(d_closeOk, false);
                d_state = State::CLOSED;
            }
        }
        else {
            LOG_WARN << "Incorrect method(" << method.methodType
                     << ") received from " << direction
                     << "after sending Close to client. "
                        "Should be either Close or CloseOK sent by client. ";
            // discarding unexpected messages per specification
        }
    } break;
    case State::SERVER_CLOSE_SENT: {
        const bool isClose = method.methodType == methods::Close::methodType();
        const bool isCloseOk =
            method.methodType == methods::CloseOk::methodType();
        if (isClose || isCloseOk) {
            LOG_TRACE
                << "Received Close/CloseOk after sending Close to server.";
            if (isClose) {
                sendResponse(d_closeOk,
                             direction ==
                                 FlowType::INGRESS);  // send CloseOk back
            }
            if (direction == FlowType::EGRESS) {
                d_state = State::CLOSED;
            }
        }
        else {
            LOG_WARN << "Incorrect method(" << method.methodType
                     << ") received from " << direction
                     << " after sending Close to server. "
                        "Should be either Close or CloseOK.";
            // discarding unexpected messages per specification
        }
    }; break;
    case State::AWAITING_PROTOCOL_HEADER:
    case State::EXPECTING_CLOSE:
    case State::ERROR:
    case State::CLOSED:
        // No Op
        break;

    };  // switch
}

void Connector::synthesizeClose(bool sendToIngressSide)
{
    d_state = sendToIngressSide ? State::CLIENT_CLOSE_SENT
                                : State::SERVER_CLOSE_SENT;
    synthesizeMessage(
        d_close, sendToIngressSide, Reply::Codes::reply_success, "OK");
}

void Connector::synthesizeCloseError(bool sendToIngressSide)
{
    synthesizeMessage(d_close,
                      sendToIngressSide,
                      Reply::Codes::channel_error,
                      "ERROR: Expected CloseOk reply");
}

void Connector::synthesizeCustomCloseError(bool             sendToIngressSide,
                                           uint16_t         code,
                                           std::string_view text)
{
    synthesizeMessage(d_close, sendToIngressSide, code, text);
}

Buffer Connector::outBuffer()
{
    return d_buffer;
}

void Connector::resetOutBuffer()
{
    d_buffer = Buffer();
}

bool Connector::sendToIngressSide()
{
    return d_sendToIngressSide;
}

void Connector::setConnectionCreationHandler(
    const std::function<void()> &handler)
{
    d_connectionCreationHandler = handler;
}

void Connector::setConnectionReadyHandler(const std::function<void()> &handler)
{
    d_connectionReadyHandler = handler;
}

template <typename T>
void Connector::sendResponse(const T &response, bool sendToIngressSide)
{
    // Until we know the message size we have to assume it's the largest
    // possible message size, after encoding the message we'll know the exact
    // frame size.
    BufferHandle tempBuffer;
    d_bufferPool_p->acquireBuffer(&tempBuffer, Frame::getMaxFrameSize());

    Buffer buildResponse(tempBuffer.data(), tempBuffer.size());
    if (Method::encode(buildResponse, response)) {
        Frame f;
        f.type                           = 1;
        f.channel                        = 0;
        f.payload                        = buildResponse.originalPtr();
        f.length                         = buildResponse.offset();
        std::size_t returnedData         = 0;
        std::size_t existingResponseData = d_buffer.size();
        std::size_t newLength            = f.length + Frame::frameOverhead();

        if (existingResponseData > 0) {
            // Expand and copy over the existing data in the reply buffer, this
            // happens when we're sending multiple messages in a row, within
            // the same network write, such as TuneOK + Open.
            BufferHandle expandedBuffer;
            d_bufferPool_p->acquireBuffer(&expandedBuffer,
                                          newLength + existingResponseData);
            memcpy(expandedBuffer.data(),
                   d_synthesizedReplyBuffer.data(),
                   existingResponseData);
            d_synthesizedReplyBuffer.swap(expandedBuffer);
        }
        else {
            d_bufferPool_p->acquireBuffer(&d_synthesizedReplyBuffer,
                                          newLength);
        }

        auto startPtr = static_cast<char *>(d_synthesizedReplyBuffer.data()) +
                        existingResponseData;
        if (Frame::encode(startPtr, &returnedData, f)) {
            // set the output buffer
            d_buffer            = Buffer(d_synthesizedReplyBuffer.data(),
                              existingResponseData + returnedData);
            d_sendToIngressSide = sendToIngressSide;

            LOG_TRACE << "Connector sendResponse: " << response;
        }
        else {
            LOG_FATAL << "Cannot encode frame: " << response;
        }
    }
    else {
        LOG_FATAL << "Cannot encode response: " << response;
    }
}

void Connector::synthesizeMessage(methods::Close  &replyMethod,
                                  bool             sendToIngressSide,
                                  uint64_t         code,
                                  std::string_view text)
{
    d_buffer = Buffer();  // forget inbound buffer
    replyMethod.setReply(code, std::string(text));
    sendResponse(replyMethod, sendToIngressSide);
}

void Connector::synthesizeProtocolHeader()
{
    auto protocolSize = Constants::protocolHeaderLength();

    // Zero protocol size would always be a bug, which should be caught in
    // development
    assert(protocolSize > 0u);

    d_bufferPool_p->acquireBuffer(&d_synthesizedReplyBuffer, protocolSize);
    Buffer tempBuffer = Buffer(d_synthesizedReplyBuffer.data(), protocolSize);

    tempBuffer.writeIn(protocolHeader);

    d_buffer = tempBuffer.currentData();
}

void Connector::synthesizeProxyProtocolHeader(
    const std::string &proxyProtocolHeader)
{
    auto protocolSize = proxyProtocolHeader.size();

    // Zero protocol size would always be a bug, which should be caught in
    // development
    assert(protocolSize > 0u);

    d_bufferPool_p->acquireBuffer(&d_synthesizedReplyBuffer, protocolSize);
    Buffer tempBuffer = Buffer(d_synthesizedReplyBuffer.data(), protocolSize);

    tempBuffer.writeIn(
        Buffer(proxyProtocolHeader.data(), proxyProtocolHeader.size()));

    d_buffer = tempBuffer.currentData();
}

const FieldTable Connector::getClientProperties() const
{
    return d_startOk.properties();
}

const std::pair<const std::string, const std::string>
Connector::getAuthMechanismCredentials() const
{
    return std::make_pair(d_startOk.mechanism(), d_startOk.response());
}

void Connector::setAuthMechanismCredentials(std::string_view authMechanism,
                                            std::string_view credentials)
{
    d_startOk.setAuthMechanism(authMechanism);
    d_startOk.setCredentials(credentials);
}

void Connector::setAuthReasonAsClientProperties(std::string_view reason)
{
    d_startOk.properties().pushField("amqpprox_auth",
                                     FieldValue('S', std::string(reason)));
}

}
}
