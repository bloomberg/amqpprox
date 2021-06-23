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
#include <amqpprox_method.h>

namespace Bloomberg {
namespace amqpprox {

bool Method::decode(Method *method, const void *buffer, std::size_t bufferLen)
{
    const uint8_t *buf = static_cast<const uint8_t *>(buffer);
    memcpy(&method->classType, buf, sizeof(method->classType));
    memcpy(&method->methodType,
           buf + sizeof(method->classType),
           sizeof(method->methodType));
    method->payload =
        buf + sizeof(method->methodType) + sizeof(method->classType);
    method->length =
        bufferLen - sizeof(method->methodType) - sizeof(method->classType);
    return true;
}

}
}
