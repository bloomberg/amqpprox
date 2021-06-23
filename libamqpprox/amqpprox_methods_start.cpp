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
#include <amqpprox_methods_start.h>

#include <amqpprox_buffer.h>
#include <amqpprox_types.h>

#include <boost/algorithm/string/join.hpp>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

Start::Start(uint8_t                         versionMajor,
             uint8_t                         versionMinor,
             const FieldTable &              properties,
             const std::vector<std::string> &mechanisms,
             const std::vector<std::string> &locales)
: d_versionMajor(versionMajor)
, d_versionMinor(versionMinor)
, d_properties(properties)
{
    d_locales    = boost::algorithm::join(locales, " ");
    d_mechanisms = boost::algorithm::join(mechanisms, " ");
}

bool Start::decode(Start *start, Buffer &buffer)
{
    if (buffer.available() < 2) {
        return false;
    }

    start->d_versionMajor = buffer.copy<boost::endian::big_uint8_t>();
    start->d_versionMinor = buffer.copy<boost::endian::big_uint8_t>();

    return Types::decodeFieldTable(&start->d_properties, buffer) &&
           Types::decodeLongString(&start->d_mechanisms, buffer) &&
           Types::decodeLongString(&start->d_locales, buffer);
}

bool Start::encode(Buffer &buffer, const Start &start)
{
    return buffer.writeIn<boost::endian::big_uint8_t>(start.versionMajor()) &&
           buffer.writeIn<boost::endian::big_uint8_t>(start.versionMinor()) &&
           Types::encodeFieldTable(buffer, start.properties()) &&
           Types::encodeLongString(buffer, start.mechanisms()) &&
           Types::encodeLongString(buffer, start.locales());
}

std::ostream &operator<<(std::ostream &os, const Start &startMethod)
{
    os << "Start = [version:" << (int)startMethod.versionMajor() << "."
       << (int)startMethod.versionMinor()
       << ", properties:" << startMethod.properties()
       << ", mechanisms:" << startMethod.mechanisms()
       << ", locale:" << startMethod.locales() << "]";
    return os;
}

bool operator==(const Start &lhs, const Start &rhs)
{
    if (lhs.versionMajor() != rhs.versionMajor()) {
        return false;
    }
    if (lhs.versionMinor() != rhs.versionMinor()) {
        return false;
    }
    if (lhs.locales() != rhs.locales()) {
        return false;
    }
    if (lhs.mechanisms() != rhs.mechanisms()) {
        return false;
    }
    if (lhs.properties() != rhs.properties()) {
        return false;
    }
    return true;
}

}
}
}
