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
#ifndef BLOOMBERG_AMQPPROX_SIMPLECONNECTIONSELECTOR
#define BLOOMBERG_AMQPPROX_SIMPLECONNECTIONSELECTOR

#include <amqpprox_connectionselector.h>

#include <amqpprox_robinbackendselector.h>

#include <memory>

namespace Bloomberg {
namespace amqpprox {

class ConnectionManager;
class SessionState;

class SimpleConnectionSelector : public ConnectionSelector {
  private:
    // DATA
    RobinBackendSelector d_selector;
    int                  d_currentIndex;

  public:
    // CREATORS
    SimpleConnectionSelector();
    virtual ~SimpleConnectionSelector() override = default;

    // MANIPULATORS
    virtual int
    acquireConnection(std::shared_ptr<ConnectionManager> *connectionOut,
                      const SessionState &                state) override;
};

}
}

#endif
