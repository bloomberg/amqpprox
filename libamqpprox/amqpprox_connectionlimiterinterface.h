/*
** Copyright 2022 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONLIMITERINTERFACE
#define BLOOMBERG_AMQPPROX_CONNECTIONLIMITERINTERFACE

#include <string>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Provide a pure virtual interface for connection limiter
 */
class ConnectionLimiterInterface {
  public:
    virtual ~ConnectionLimiterInterface() = default;

    // MANIPULATORS
    /**
     * \brief Decide whether the current request should be allowed or not based
     * on the capacity
     */
    virtual bool allowNewConnection() = 0;

    /**
     * \brief Called when an aquired connection is closed. Useful for changing
     * the state of the limiter based on close connection event.
     */
    virtual void connectionClosed() {}

    // ACCESSORS
    /**
     * \return information about connection limiter as a string
     */
    virtual std::string toString() const = 0;
};

}
}

#endif
