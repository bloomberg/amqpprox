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
#ifndef BLOOMBERG_AMQPPROX_METHODS_TUNEOK
#define BLOOMBERG_AMQPPROX_METHODS_TUNEOK

#include <boost/endian/arithmetic.hpp>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

class TuneOk {
    boost::endian::big_uint16_t d_channelMax;
    boost::endian::big_uint32_t d_frameMax;
    boost::endian::big_uint16_t d_heartbeatInterval;

  public:
    uint16_t channelMax() const { return d_channelMax; }

    uint32_t frameMax() const { return d_frameMax; }

    uint16_t heartbeatInterval() const { return d_heartbeatInterval; }

    static bool decode(TuneOk *tune, Buffer &buffer);

    static bool encode(Buffer &buffer, const TuneOk &tuneOk);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 31; }
};

std::ostream &operator<<(std::ostream &os, const TuneOk &tuneOkMethod);

}
}
}

#endif
