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
#include <amqpprox_methods_tuneok.h>

#include <amqpprox_buffer.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

using boost::endian::big_uint16_t;
using boost::endian::big_uint32_t;

bool TuneOk::decode(TuneOk *tuneOk, Buffer &buffer)
{
    if (sizeof(tuneOk->d_channelMax) + sizeof(tuneOk->d_frameMax) +
            sizeof(tuneOk->d_heartbeatInterval) >
        buffer.available()) {
        return false;
    }

    tuneOk->d_channelMax        = buffer.copy<big_uint16_t>();
    tuneOk->d_frameMax          = buffer.copy<big_uint32_t>();
    tuneOk->d_heartbeatInterval = buffer.copy<big_uint16_t>();
    return true;
}

bool TuneOk::encode(Buffer &buffer, const TuneOk &tuneOk)
{
    return buffer.writeIn<big_uint16_t>(tuneOk.channelMax()) &&
           buffer.writeIn<big_uint32_t>(tuneOk.frameMax()) &&
           buffer.writeIn<big_uint16_t>(tuneOk.heartbeatInterval());
}

std::ostream &operator<<(std::ostream &os, const TuneOk &tuneOkMethod)
{
    os << "TuneOk = ["
       << "channelMax: " << tuneOkMethod.channelMax()
       << ", frameMax: " << tuneOkMethod.frameMax()
       << ", heartbeatInterval: " << tuneOkMethod.heartbeatInterval() << "]";
    return os;
}

}
}
}
