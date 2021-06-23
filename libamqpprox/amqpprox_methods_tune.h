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
#ifndef BLOOMBERG_AMQPPROX_METHODS_TUNE
#define BLOOMBERG_AMQPPROX_METHODS_TUNE

#include <amqpprox_constants.h>
#include <boost/endian/arithmetic.hpp>

#include <iosfwd>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

/**
 * \brief Represents AMQP Connection TUNE method
 */
class Tune {
    // https://github.com/rabbitmq/rabbitmq-server/issues/1593
    boost::endian::big_uint16_t d_channelMax{Constants::channelMaximum()};
    boost::endian::big_uint32_t d_frameMax{0};
    boost::endian::big_uint16_t d_heartbeatInterval{0};

  public:
    Tune() = default;
    Tune(uint16_t channelMax, uint32_t frameMax, uint16_t heartbeatInterval);

    uint16_t channelMax() const { return d_channelMax; }

    uint32_t frameMax() const { return d_frameMax; }

    uint16_t heartbeatInterval() const { return d_heartbeatInterval; }

    /**
     * \brief Decode specified buffer and copy the data into tune method
     */
    static bool decode(Tune *tune, Buffer &buffer);

    /**
     * \brief Encode tune method and write the data into buffer
     */
    static bool encode(Buffer &buffer, const Tune &tune);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 30; }
};

std::ostream &operator<<(std::ostream &os, const Tune &tuneMethod);

}
}
}

#endif
