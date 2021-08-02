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

#include <amqpprox_authresponsedata.h>

#include <string_view>

namespace Bloomberg {
namespace amqpprox {

AuthResponseData::AuthResponseData(const AuthResult &authResult,
                                   std::string_view  reason,
                                   std::string_view  authMechanism,
                                   std::string_view  credentials)
: d_authResult(authResult)
, d_reason(reason)
, d_authMechanism(authMechanism)
, d_credentials(credentials)
{
}

}
}
