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
#ifndef BLOOMBERG_AMQPPROX_METHODS_SECURE
#define BLOOMBERG_AMQPPROX_METHODS_SECURE

#include <iosfwd>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

class Secure {
    std::string d_challenge;

  public:
    const std::string &challenge() const { return d_challenge; }

    static bool decode(Secure *start, Buffer &buffer);

    constexpr inline static int methodType() { return 20; }
};

std::ostream &operator<<(std::ostream &os, const Secure &item);

}
}
}

#endif
