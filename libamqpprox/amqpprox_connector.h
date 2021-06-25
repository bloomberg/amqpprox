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

#include <functional>
#include <string_view>

namespace Bloomberg {
namespace amqpprox {

class BufferPool;
class EventSource;
class SessionState;

class Connector {
  public:
    enum class State {
        AWAITING_PROTOCOL_HEADER,
        START_SENT,
        SECURE_SENT,
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
    SessionState *        d_sessionState_p;  // HELD NOT OWNED
    EventSource *         d_eventSource_p;   // HELD NOT OWNED
    BufferPool *          d_bufferPool_p;    // HELD NOT OWNED
    BufferHandle          d_synthesizedReplyBuffer;
    Buffer                d_buffer;
    std::function<void()> d_connectionCreationHandler;
    std::function<void()> d_connectionReadyHandler;
    bool                  d_sendToIngressSide;
    bool                  d_reconnection;
    std::string           d_localHostname;

    template <typename T>
    void sendResponse(const T &response, bool sendToIngressSide);

    template <typename TReply, typename TMethod>
    inline void synthesizeMessage(TMethod &replyMethod,
                                  bool     sendToIngressSide);

  public:
    Connector(SessionState *   sessionState,
              EventSource *    eventSource,
              BufferPool *     bufferPool,
              std::string_view localHostname);

    inline State state() const;
    void         receive(const Buffer &buffer);
    void         receive(const Method &method, FlowType direction);
    void setConnectionCreationHandler(const std::function<void()> &handler);
    void setConnectionReadyHandler(const std::function<void()> &handler);

    void synthesizeClose(bool sendToIngressSide);
    void synthesizeCloseError(bool sendToIngressSide);
    void synthesizeProtocolHeader();
    void synthesizeProxyProtocolHeader(const std::string &proxyProtocolHeader);

    Buffer outBuffer();
    bool   sendToIngressSide();
};

inline Connector::State Connector::state() const
{
    return d_state;
}

}
}

#endif
