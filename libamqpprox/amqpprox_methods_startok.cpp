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
#include <amqpprox_methods_startok.h>

#include <amqpprox_types.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {
namespace methods {

bool StartOk::decode(StartOk *startOk, Buffer &buffer)
{
    return Types::decodeFieldTable(&startOk->d_properties, buffer) &&
           Types::decodeShortString(&startOk->d_mechanism, buffer) &&
           Types::decodeLongString(&startOk->d_response, buffer) &&
           Types::decodeShortString(&startOk->d_locale, buffer);
}

bool StartOk::encode(Buffer &buffer, const StartOk &startOk)
{
    return Types::encodeFieldTable(buffer, startOk.d_properties) &&
           Types::encodeShortString(buffer, startOk.d_mechanism) &&
           Types::encodeLongString(buffer, startOk.d_response) &&
           Types::encodeShortString(buffer, startOk.d_locale);
}

std::ostream &operator<<(std::ostream &os, const StartOk &okMethod)
{
    os << "StartOk = [properties:" << okMethod.properties()
       << ", mechanism:" << okMethod.mechanism()
       << ", response:" << okMethod.response()
       << ", locale:" << okMethod.locale() << "]";
    return os;
}

}
}
}
