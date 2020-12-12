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
#include <amqpprox_flowtype.h>
#include <amqpprox_packetprocessor.h>

#include <amqpprox_buffer.h>
#include <amqpprox_connector.h>
#include <amqpprox_frame.h>
#include <amqpprox_logging.h>
#include <amqpprox_method.h>
#include <amqpprox_methods_close.h>
#include <amqpprox_methods_closeok.h>
#include <amqpprox_methods_open.h>
#include <amqpprox_methods_openok.h>
#include <amqpprox_methods_secure.h>
#include <amqpprox_methods_secureok.h>
#include <amqpprox_methods_start.h>
#include <amqpprox_methods_startok.h>
#include <amqpprox_methods_tune.h>
#include <amqpprox_methods_tuneok.h>
#include <amqpprox_sessionstate.h>

#include <iomanip>
#include <iostream>

namespace Bloomberg {
namespace amqpprox {

PacketProcessor::PacketProcessor(SessionState &state, Connector &connector)
: d_state(state)
, d_connector(connector)
{
}

void PacketProcessor::process(FlowType direction, const Buffer &readBuffer)
{
    std::size_t remaining = readBuffer.offset();
    const void *nextFrame = readBuffer.originalPtr();

    if (d_connector.state() == Connector::State::AWAITING_PROTOCOL_HEADER) {
        d_connector.receive(readBuffer.currentData());

        if (d_connector.sendToIngressSide()) {
            d_ingressWriteBuffer = d_connector.outBuffer();
            d_egressWriteBuffer  = Buffer();
        }
        else {
            d_ingressWriteBuffer = Buffer();
            d_egressWriteBuffer  = d_connector.outBuffer();
        }

        return;
    }

    Frame frame;
    int   frameCount = 0;
    while (remaining >= Frame::frameOverhead()) {
        const void *currFrame = nextFrame;
        bool        decodable = Frame::decode(
            &frame, &nextFrame, &remaining, currFrame, remaining);

        if (!decodable) {
            break;
        }

        ++frameCount;

        if (1 == frame.type && d_connector.state() != Connector::State::OPEN) {
            Method method;
            Method::decode(&method, frame.payload, frame.length);

            LOG_TRACE << ((direction == FlowType::INGRESS) ? "->" : "<-")
                      << " Frame: TYPE=" << (int)frame.type
                      << " CHANNEL=" << frame.channel
                      << " LEN=" << frame.length << " REM=" << remaining
                      << " CLASS=" << method.classType
                      << " METH=" << method.methodType;

            if (method.classType == 10) {  // Connection
                d_connector.receive(method, direction);

                if (d_connector.sendToIngressSide()) {
                    d_ingressWriteBuffer = d_connector.outBuffer();
                    d_egressWriteBuffer  = Buffer();
                }
                else {
                    d_ingressWriteBuffer = Buffer();
                    d_egressWriteBuffer  = d_connector.outBuffer();
                }
            }
        }
    }

    // Pass through the data
    if (d_connector.state() == Connector::State::OPEN) {
        size_t byteCount = readBuffer.offset() - remaining;

        LOG_TRACE << ((direction == FlowType::INGRESS) ? "->" : "<-")
                  << " Passthrough " << byteCount << " bytes";

        if (direction == FlowType::INGRESS) {
            d_egressWriteBuffer  = Buffer(readBuffer.originalPtr(), byteCount);
            d_ingressWriteBuffer = Buffer();
            d_state.incrementIngressTotals(frameCount, byteCount);
        }
        else {
            d_egressWriteBuffer  = Buffer();
            d_ingressWriteBuffer = Buffer(readBuffer.originalPtr(), byteCount);
            d_state.incrementEgressTotals(frameCount, byteCount);
        }
    }

    d_remainingBuffer = Buffer(nextFrame, remaining);
}

}
}
