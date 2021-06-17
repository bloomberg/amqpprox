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
#include <amqpprox_connectorutil.h>
#include <amqpprox_constants.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_fieldtable.h>
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
const Buffer protocolHeader(Constants::protocolHeader(),
                            Constants::protocolHeaderLength());
const Buffer legacyProtocolHeader(Constants::legacyProtocolHeader(),
                                  Constants::legacyProtocolHeaderLength());
}

Connector::Connector(SessionState *   sessionState,
                     EventSource *    eventSource,
                     BufferPool *     bufferPool,
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
    if (d_state == State::OPEN) {
        // Once the session is fully open and passing through messages, we can
        // discard the synthesized reply buffers
        d_synthesizedReplyBuffer.release();

        if (method.methodType == methods::CloseOk::methodType() ||
            method.methodType == methods::Close::methodType()) {
            // We catch both Close and CloseOk here because of catching one
            // side intentionally closing the connection, and the client
            // misbehaving and not respecting it with a CloseOk. This should
            // still count as graceful.
            LOG_TRACE << "Received Close/CloseOk";
            d_state = State::CLOSED;
        }
    }
    // Acting as a server
    else if (d_state == State::START_SENT) {
        if (method.methodType != methods::StartOk::methodType()) {
            // TODO error
        }

        if (!methods::StartOk::decode(&d_startOk, methodPayload)) {
            // TODO error
        }

        LOG_TRACE << "Received: " << d_startOk;

        sendResponse(d_synthesizedTune, true);
        d_state = State::TUNE_SENT;
    }
    else if (d_state == State::TUNE_SENT) {
        if (method.methodType != methods::TuneOk::methodType()) {
            // TODO error
        }

        if (!methods::TuneOk::decode(&d_tuneOk, methodPayload)) {
            // TODO error
        }

        LOG_TRACE << "Tuned: " << d_tuneOk;

        d_state = State::AWAITING_OPEN;
    }
    else if (d_state == State::AWAITING_OPEN) {
        if (method.methodType != methods::Open::methodType()) {
            // TODO error
        }

        if (!methods::Open::decode(&d_open, methodPayload)) {
            // TODO error
        }

        d_sessionState_p->setVirtualHost(d_open.virtualHost());
        d_eventSource_p->connectionVhostEstablished().emit(
            d_sessionState_p->id(), d_open.virtualHost());

        LOG_TRACE << "Open: " << d_open;

        d_state = State::AWAITING_CONNECTION;
        d_connectionCreationHandler();
    }
    // Acting as a client
    else if (d_state == State::AWAITING_CONNECTION) {
        if (method.methodType != methods::Start::methodType()) {
            // TODO error
        }

        if (!methods::Start::decode(&d_receivedStart, methodPayload)) {
            // TODO error
        }

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
            outboundLocalPort);

        sendResponse(d_startOk, false);
        d_state = State::STARTOK_SENT;
    }
    else if (d_state == State::STARTOK_SENT) {
        if (method.methodType != methods::Tune::methodType()) {
            // TODO error
        }

        if (!methods::Tune::decode(&d_receivedTune, methodPayload)) {
            // TODO error
        }

        LOG_TRACE << "Received Tune: " << d_receivedTune;

        sendResponse(d_tuneOk, false);
        sendResponse(d_open, false);
        d_state = State::OPEN_SENT;
    }
    else if (d_state == State::OPEN_SENT) {
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
    }
    else if (d_state == State::CLIENT_CLOSE_SENT) {
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
    }
    else if (d_state == State::SERVER_CLOSE_SENT) {
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
    }
}

void Connector::synthesizeClose(bool sendToIngressSide)
{
    d_state = sendToIngressSide ? State::CLIENT_CLOSE_SENT
                                : State::SERVER_CLOSE_SENT;
    synthesizeMessage<Reply::OK>(d_close, sendToIngressSide);
}

void Connector::synthesizeCloseError(bool sendToIngressSide)
{
    synthesizeMessage<Reply::CloseOkExpected>(d_close, sendToIngressSide);
}

Buffer Connector::outBuffer()
{
    return d_buffer;
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

template <typename TReply, typename TMethod>
inline void Connector::synthesizeMessage(TMethod &replyMethod,
                                         bool     sendToIngressSide)
{
    d_buffer = Buffer();  // forget inbound buffer
    replyMethod.setReply(TReply::CODE, TReply::TEXT);
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

}
}
