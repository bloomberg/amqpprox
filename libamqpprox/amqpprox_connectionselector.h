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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONSELECTOR
#define BLOOMBERG_AMQPPROX_CONNECTIONSELECTOR

#include <memory>

namespace Bloomberg {
namespace amqpprox {

class ConnectionManager;
class SessionState;

class ConnectionSelector {
  public:
    virtual ~ConnectionSelector() = default;

    virtual int
    acquireConnection(std::shared_ptr<ConnectionManager> *connectionOut,
                      const SessionState &                state) = 0;
    ///< Acquire a connection from the specified session `state` and set
    ///< `connectionOut` to be a `ConnectionManager` instance tracking the
    ///< connection attempt.
    ///< Returns zero on success, or a non-zero value otherwise.
};

}
}

#endif
