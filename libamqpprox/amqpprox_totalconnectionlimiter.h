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
#ifndef BLOOMBERG_AMQPPROX_TOTALCONNECTIONLIMITER
#define BLOOMBERG_AMQPPROX_TOTALCONNECTIONLIMITER

#include <amqpprox_connectionlimiterinterface.h>

#include <string>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief The class will impose total allowed connection limit based on
 * provided connection limit. Implements the ConnectionLimiterInterface
 * interface
 */
class TotalConnectionLimiter : public ConnectionLimiterInterface {
    // Maximum total connection limit
    uint32_t d_totalConnectionLimit;

    // connection count
    uint32_t d_connectionCount;

  public:
    // CREATORS
    explicit TotalConnectionLimiter(uint32_t totalConnectionLimit);

    // MANIPULATORS
    /**
     * \brief Decide whether the current connection request should be allowed
     * based on total connection limit
     *
     * \note The method should always be called in thread-safe manner/serially,
     * otherwise the connection counter value will not be maintained accurately
     */
    virtual bool allowNewConnection() override;

    /**
     * \brief Called when an aquired connection is closed. Useful for changing
     * the state of the limiter based on close connection event.
     */
    virtual void connectionClosed() override;

    // ACCESSORS
    /**
     * \return Information about connection limiter as a string
     */
    virtual std::string toString() const override;

    /**
     * \return the total connection limit (total allowed connections)
     */
    uint32_t getTotalConnectionLimit() const;

    /**
     * \return the current connection count
     */
    uint32_t getConnectionCount() const;
};

}
}

#endif
