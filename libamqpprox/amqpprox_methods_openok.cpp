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
#include <amqpprox_methods_openok.h>

#include <amqpprox_buffer.h>
#include <amqpprox_types.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

bool OpenOk::decode(OpenOk *open, Buffer &buffer)
{
    std::string dummy;
    return Types::decodeShortString(&dummy, buffer);
}

bool OpenOk::encode(Buffer &buffer, const OpenOk &open)
{
    return Types::encodeShortString(buffer, "");
}

std::ostream &operator<<(std::ostream &os, const OpenOk &method)
{
    os << "OpenOk = []";
    return os;
}

}
}
}
