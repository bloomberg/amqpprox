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
#ifndef BLOOMBERG_AMQPPROX_CONNECTOR
#define BLOOMBERG_AMQPPROX_CONNECTOR

#include <amqpprox_buffer.h>
#include <amqpprox_bufferhandle.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_flowtype.h>
#include <amqpprox_method.h>
#include <amqpprox_methods_close.h>
#include <amqpprox_methods_closeok.h>
#include <amqpprox_methods_open.h>
#include <amqpprox_methods_openok.h>
#include <amqpprox_methods_start.h>
#include <amqpprox_methods_startok.h>
#include <amqpprox_methods_tune.h>
#include <amqpprox_methods_tuneok.h>
#include <amqpprox_reply.h>

#include <functional>
#include <string_view>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

class BufferPool;
class EventSource;
class SessionState;

/**
 * \brief Works as a bridge between client and broker. It also holds logic to
 * perform complete synthesize handshake from client end to server end in
 * specific way.
 *
 * It does the handshake with the client and get through to the point of
 * knowing which virtual host the connection is for. Once the virtual host is
 * known the `ConnectionSelector` is invoked to determine where to make the
 * egress connection. This is resolved using boost ASIO, and the same Connector
 * object is used to do the egress handshaking with the broker. Once the
 * `OpenOk` message has been passed to the connector the Session is fully
 * established and all future reads and writes are passed through unchanged.
 */
class Connector {
  public:
    /**
     * \brief Represents connection status
     */
    enum class State {
        AWAITING_PROTOCOL_HEADER,
        START_SENT,
        TUNE_SENT,
        AWAITING_OPEN,
        AWAITING_CONNECTION,
        STARTOK_SENT,
        OPEN_SENT,
        OPEN,
        EXPECTING_CLOSE,
        CLOSED,
        CLIENT_CLOSE_SENT,
        SERVER_CLOSE_SENT,
        ERROR
    };

  private:
    State                 d_state;
    methods::Start        d_synthesizedStart;
    methods::Start        d_receivedStart;
    methods::StartOk      d_startOk;
    methods::Tune         d_synthesizedTune;
    methods::Tune         d_receivedTune;
    methods::TuneOk       d_tuneOk;
    methods::Open         d_open;
    methods::OpenOk       d_openOk;
    methods::Close        d_close;
    methods::CloseOk      d_closeOk;
    SessionState         *d_sessionState_p;  // HELD NOT OWNED
    EventSource          *d_eventSource_p;   // HELD NOT OWNED
    BufferPool           *d_bufferPool_p;    // HELD NOT OWNED
    BufferHandle          d_synthesizedReplyBuffer;
    Buffer                d_buffer;
    std::function<void()> d_connectionCreationHandler;
    std::function<void()> d_connectionReadyHandler;
    bool                  d_sendToIngressSide;
    bool                  d_reconnection;
    std::string           d_localHostname;

    template <typename T>
    void sendResponse(const T &response, bool sendToIngressSide);

    inline void synthesizeMessage(methods::Close  &replyMethod,
                                  bool             sendToIngressSide,
                                  uint64_t         code,
                                  std::string_view text);

  public:
    Connector(SessionState    *sessionState,
              EventSource     *eventSource,
              BufferPool      *bufferPool,
              std::string_view localHostname);

    /**
     * \return state of the connection
     */
    inline State state() const;

    /**
     * \brief This method mainly receive AMQP protocol header buffer. And after
     * receiving, it sends the AMQP method Start to clients
     */
    void receive(const Buffer &buffer);

    /**
     * \brief Receive decoded AMQP method from `PacketProcessor` with the data
     * flow direction (ingress/egress). This method is responsible for
     * synthesize handshake. And change the state of the connection
     * accordingly.
     * \param method an AMQP connection method
     * \param direction specifies direction of the data flow (ingress/egress)
     */
    void receive(const Method &method, FlowType direction);

    /**
     * \brief Set connection creation handler
     */
    void setConnectionCreationHandler(const std::function<void()> &handler);

    /**
     * \brief Set connection creation handler
     */
    void setConnectionReadyHandler(const std::function<void()> &handler);

    /**
     * \brief Send AMQP connection Close method with OK status to client/server
     * based on specified direction
     * \param sendToIngressSide true for communicating with client and false
     * for communicating with server
     */
    void synthesizeClose(bool sendToIngressSide);

    /**
     * \brief Send AMQP connection Close method with ERROR status to
     * client/server based on specified direction
     * \param sendToIngressSide true for communicating with client and false
     * for communicating with server
     */
    void synthesizeCloseError(bool sendToIngressSide);

    /**
     * \brief Send AMQP connection Close method with custom ERROR code and text
     * to client/server based on specified direction
     * \param sendToIngressSide true for communicating with client and false
     * for communicating with server
     * \param code custom error code
     * \param text custom error text
     */
    void synthesizeCustomCloseError(bool             sendToIngressSide,
                                    uint16_t         code,
                                    std::string_view text);

    /**
     * \brief Synthesize AMQP protocol header buffer, which will eventually be
     * sent to server(broker).
     */
    void synthesizeProtocolHeader();

    /**
     * \brief Synthesize AMQP proxy protocol header buffer, which will
     * eventually be sent to server(broker).
     */
    void synthesizeProxyProtocolHeader(const std::string &proxyProtocolHeader);

    /**
     * \return current buffer
     */
    Buffer outBuffer();

    /**
     * \brief Reset current buffer
     */
    void resetOutBuffer();

    /**
     * \return the current direction of the data flow (ingree/egress)
     */
    bool sendToIngressSide();

    /**
     * \brief AMQP client sends client properties using START-OK connection
     * method. The method extracts properties information from that method
     * fields.
     * \return client properties as a Fieldtable
     */
    const FieldTable getClientProperties() const;

    /**
     * \brief AMQP client sends auth mechansim and credential information using
     * START-OK connection method. The method extracts mechanism and credential
     * information from that method fields.
     * \return pair of AMQP authentication mechanism, AMQP response field
     */
    const std::pair<const std::string, const std::string>
    getAuthMechanismCredentials() const;

    /**
     * \brief Set different authentication mechanism and credentials for AMQP
     * START-OK connection method, which will be sent to server for
     * authentication and authorization
     * \param authMechanism of AMQP authentication mechanism
     * \param credentials data for AMQP response field
     */
    void setAuthMechanismCredentials(std::string_view authMechanism,
                                     std::string_view credentials);

    /**
     * \brief Set the reason/detail for allowing clients to connect to amqpprox
     * proxy, if external auth service is used.
     * \param reason for allowing connection. The reason field is returned by
     * external configured auth service inside AuthResponse protobuf response.
     */
    void setAuthReasonAsClientProperties(std::string_view reason);
};

inline Connector::State Connector::state() const
{
    return d_state;
}

}
}

#endif
