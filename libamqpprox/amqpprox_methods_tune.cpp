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
#include <amqpprox_methods_tune.h>

#include <amqpprox_buffer.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

using boost::endian::big_uint16_t;
using boost::endian::big_uint32_t;

Tune::Tune(uint16_t channelMax, uint32_t frameMax, uint16_t heartbeatInterval)
: d_channelMax(channelMax)
, d_frameMax(frameMax)
, d_heartbeatInterval(heartbeatInterval)
{
}

bool Tune::decode(Tune *tune, Buffer &buffer)
{
    if (sizeof(tune->d_channelMax) + sizeof(tune->d_frameMax) +
            sizeof(tune->d_heartbeatInterval) >
        buffer.available()) {
        return false;
    }

    tune->d_channelMax        = buffer.copy<big_uint16_t>();
    tune->d_frameMax          = buffer.copy<big_uint32_t>();
    tune->d_heartbeatInterval = buffer.copy<big_uint16_t>();
    return true;
}

bool Tune::encode(Buffer &buffer, const Tune &tune)
{
    return buffer.writeIn<big_uint16_t>(tune.channelMax()) &&
           buffer.writeIn<big_uint32_t>(tune.frameMax()) &&
           buffer.writeIn<big_uint16_t>(tune.heartbeatInterval());
}

std::ostream &operator<<(std::ostream &os, const Tune &tuneMethod)
{
    os << "Tune = ["
       << "channelMax: " << tuneMethod.channelMax()
       << ", frameMax: " << tuneMethod.frameMax()
       << ", heartbeatInterval: " << tuneMethod.heartbeatInterval() << "]";
    return os;
}

}
}
}
