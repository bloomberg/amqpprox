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
#ifndef BLOOMBERG_AMQPPROX_METHODS_SECUREOK
#define BLOOMBERG_AMQPPROX_METHODS_SECUREOK

#include <iosfwd>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

/**
 * \brief Represents AMQP Connection SECURE-OK method
 */
class SecureOk {
    std::string d_response;

  public:
    const std::string &response() const { return d_response; }

    /**
     * \brief Decode specified buffer and copy the data into secure-ok method
     */
    static bool decode(SecureOk *secureOk, Buffer &buffer);

    constexpr inline static int methodType() { return 21; }
};

std::ostream &operator<<(std::ostream &os, const SecureOk &item);

}
}
}

#endif
