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
#ifndef BLOOMBERG_AMQPPROX_AUTHREQUESTDATA
#define BLOOMBERG_AMQPPROX_AUTHREQUESTDATA

#include <string>
#include <string_view>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Provide a class to hold request data for authn/authz operations
 */
class AuthRequestData {
    std::string d_vhostName;
    std::string d_authMechanism;
    std::string d_credentials;

  public:
    /**
     * \brief Create and initialize object of AuthRequestData class
     * \param vhostName vhost name
     * \param authMechanism authentication mechanism field for START-OK
     * connection method
     * \param credentials response field for START-OK connection method
     */
    AuthRequestData(std::string_view vhostName,
                    std::string_view authMechanism,
                    std::string_view credentials);

    AuthRequestData();
};

}
}

#endif
