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
#include <amqpprox_methods_close.h>

#include <amqpprox_buffer.h>
#include <amqpprox_types.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

Close::Close()
: d_replyCode(0)
, d_replyString("")
, d_classId(0)
, d_methodId(0)
{
}

bool Close::decode(Close *close, Buffer &buffer)
{
    if (buffer.available() < 2) {
        return false;
    }

    close->d_replyCode = buffer.copy<boost::endian::big_uint16_t>();

    if (!Types::decodeShortString(&close->d_replyString, buffer)) {
        return false;
    }

    if (buffer.available() < 4) {
        return false;
    }

    close->d_classId  = buffer.copy<boost::endian::big_uint16_t>();
    close->d_methodId = buffer.copy<boost::endian::big_uint16_t>();

    return true;
}

bool Close::encode(Buffer &buffer, const Close &close)
{
    using boost::endian::big_uint16_t;

    return buffer.writeIn<big_uint16_t>(close.replyCode()) &&
           Types::encodeShortString(buffer, close.replyString()) &&
           buffer.writeIn<big_uint16_t>(close.classId()) &&
           buffer.writeIn<big_uint16_t>(close.methodId());
}

std::ostream &operator<<(std::ostream &os, const Close &closeMethod)
{
    os << "Close = [replyCode: " << closeMethod.replyCode()
       << ", replyString: \"" << closeMethod.replyString() << "\""
       << ", classId: " << closeMethod.classId()
       << ", methodId: " << closeMethod.methodId() << "]";
    return os;
}

}
}
}
