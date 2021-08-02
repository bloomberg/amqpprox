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
#ifndef BLOOMBERG_AMQPPROX_METHODS_STARTOK
#define BLOOMBERG_AMQPPROX_METHODS_STARTOK

#include <amqpprox_fieldtable.h>

#include <boost/endian/arithmetic.hpp>
#include <iosfwd>
#include <string>
#include <string_view>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

/**
 * \brief Represents AMQP Connection START-OK method
 */
class StartOk {
    FieldTable  d_properties;
    std::string d_mechanism;
    std::string d_response;
    std::string d_locale;

  public:
    const FieldTable &properties() const { return d_properties; }

    FieldTable &properties() { return d_properties; }

    const std::string &mechanism() const { return d_mechanism; }

    const std::string &response() const { return d_response; }

    const std::string &locale() const { return d_locale; }

    /**
     * \brief Decode specified buffer and copy the data into start-ok method
     */
    static bool decode(StartOk *startOk, Buffer &buffer);

    /**
     * \brief Encode start-ok method and write the data into buffer
     */
    static bool encode(Buffer &buffer, const StartOk &startOk);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 11; }

    /**
     * \brief Set specified AMQP client properties
     * \param clientProperties AMQP client properties
     */
    void setClientProperties(const FieldTable &clientProperties);

    /**
     * \brief Set specified AMQP authMechanism
     * \param authMechanism AMQP authentication mechanism
     */
    void setAuthMechanism(std::string_view authMechanism);

    /**
     * \brief Set specified AMQP response credentials
     * \param credentials AMQP opaque credential data
     */
    void setCredentials(std::string_view credentials);
};

std::ostream &operator<<(std::ostream &os, const StartOk &okMethod);

}
}
}

#endif
