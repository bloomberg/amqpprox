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
#ifndef BLOOMBERG_AMQPPROX_METHODS_START
#define BLOOMBERG_AMQPPROX_METHODS_START

#include <amqpprox_fieldtable.h>

#include <boost/endian/arithmetic.hpp>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

/**
 * \brief Represents AMQP Connection START method
 */
class Start {
    boost::endian::big_uint8_t d_versionMajor;
    boost::endian::big_uint8_t d_versionMinor;
    FieldTable                 d_properties;
    std::string                d_mechanisms;
    std::string                d_locales;

  public:
    Start() = default;

    Start(uint8_t                         versionMajor,
          uint8_t                         versionMinor,
          const FieldTable &              properties,
          const std::vector<std::string> &mechanisms,
          const std::vector<std::string> &locales);

    const FieldTable &properties() const { return d_properties; }

    const std::string &mechanisms() const { return d_mechanisms; }

    const std::string &locales() const { return d_locales; }

    uint8_t versionMajor() const { return d_versionMajor; }

    uint8_t versionMinor() const { return d_versionMinor; }

    /**
     * \brief Decode specified buffer and copy the data into start method
     */
    static bool decode(Start *start, Buffer &buffer);

    /**
     * \brief Encode start method and write the data into buffer
     */
    static bool encode(Buffer &buffer, const Start &start);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 10; }
};

std::ostream &operator<<(std::ostream &os, const Start &startMethod);

bool operator==(const Start &lhs, const Start &rhs);

inline bool operator!=(const Start &lhs, const Start &rhs)
{
    return !(lhs == rhs);
}

}
}
}

#endif
