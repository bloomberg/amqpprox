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

#include <amqpprox_authrequestdata.h>

namespace Bloomberg {
namespace amqpprox {

AuthRequestData::AuthRequestData(std::string_view vhostName,
                                 std::string_view authMechanism,
                                 std::string_view credentials)
: d_vhostName(vhostName)
, d_authMechanism(authMechanism)
, d_credentials(credentials)
{
}

AuthRequestData::AuthRequestData()
: d_vhostName()
, d_authMechanism()
, d_credentials()
{
}

}
}
