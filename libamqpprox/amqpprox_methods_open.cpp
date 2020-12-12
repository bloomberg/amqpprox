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
#include <amqpprox_methods_open.h>

#include <amqpprox_buffer.h>
#include <amqpprox_types.h>

#include <boost/endian/arithmetic.hpp>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

bool Open::decode(Open *open, Buffer &buffer)
{
    // Here we do not decode the reserved slots
    return Types::decodeShortString(&open->d_virtualHost, buffer);
}

bool Open::encode(Buffer &buffer, const Open &open)
{
    return Types::encodeShortString(buffer, open.virtualHost()) &&
           Types::encodeShortString(buffer, "") &&
           buffer.writeIn<boost::endian::big_uint8_t>(0);
}

std::ostream &operator<<(std::ostream &os, const Open &openMethod)
{
    os << "Open = [virtualHost: \"" << openMethod.virtualHost() << "\"]";
    return os;
}

}
}
}
