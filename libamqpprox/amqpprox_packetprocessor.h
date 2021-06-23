/*
** Copyright 2021 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_AMQPPROX_PROCESS
#define BLOOMBERG_AMQPPROX_PROCESS

#include <amqpprox_buffer.h>
#include <amqpprox_flowtype.h>

#include <cstddef>

namespace Bloomberg {
namespace amqpprox {

class Connector;
class SessionState;

class PacketProcessor {
    SessionState &d_state;
    Connector &   d_connector;
    Buffer        d_ingressWriteBuffer;
    Buffer        d_egressWriteBuffer;
    Buffer        d_remainingBuffer;

  public:
    PacketProcessor(SessionState &state, Connector &connector);

    void process(FlowType direction, const Buffer &readBuffer);

    inline Buffer remaining();
    inline Buffer ingressWrite();
    inline Buffer egressWrite();
};

inline Buffer PacketProcessor::remaining()
{
    return d_remainingBuffer;
}

inline Buffer PacketProcessor::ingressWrite()
{
    return d_ingressWriteBuffer;
}

inline Buffer PacketProcessor::egressWrite()
{
    return d_egressWriteBuffer;
}

}
}

#endif
