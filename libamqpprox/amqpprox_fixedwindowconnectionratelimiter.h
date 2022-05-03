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
#ifndef BLOOMBERG_AMQPPROX_FIXEDWINDOWCONNECTIONRATELIMITER
#define BLOOMBERG_AMQPPROX_FIXEDWINDOWCONNECTIONRATELIMITER

#include <amqpprox_connectionlimiterinterface.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief The struct will be used to mock the std::chrono::steady_clock::now()
 * for unit testing.
 */

struct LimiterClock {
    virtual ~LimiterClock() = default;
    virtual std::chrono::time_point<std::chrono::steady_clock,
                                    std::chrono::milliseconds>
    now()
    {
        return std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now());
    }
};

/**
 * \brief The class will impose fixed window connection rate limit based on
 * provided connection limit and time window. The connection rate limit will be
 * connection limit/timeWindow (average allowed connections in the specified
 * time window). allowNewConnection member function will return true or false
 * based on the rate limit calculation. Implements the LimiterInterface
 * interface
 */
class FixedWindowConnectionRateLimiter : public ConnectionLimiterInterface {
  protected:
    std::shared_ptr<LimiterClock> d_clockPtr;

  private:
    // Maximum allowed connections in particular time window
    uint32_t d_connectionLimit;

    // Time window to consider while limiting requests in miliseconds, default
    // value will be initialized with 1 second = 1000 miliseconds
    std::chrono::milliseconds d_timeWindowInMs;

    // Time when the last request is received
    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::milliseconds>
        d_lastTime;

    // Maintain the number of allowed requests in the current time window
    uint32_t d_currentCount;

  protected:
    // This constructor should only be used in unit testing to pass mock Clock
    // struct to manipulate std::chrono::steady_clock::now() value
    FixedWindowConnectionRateLimiter(
        const std::shared_ptr<LimiterClock> &clockPtr,
        uint32_t                             connectionLimit,
        uint32_t                             timeWindowInSec = 1);

  public:
    // CREATORS
    explicit FixedWindowConnectionRateLimiter(uint32_t connectionLimit,
                                              uint32_t timeWindowInSec = 1);

    virtual ~FixedWindowConnectionRateLimiter() override = default;

    // MANIPULATORS
    /**
     * \brief Decide whether the current connection request should be allowed
     * or not based on the connection limit and time window value
     *
     * \note The method should always be called in thread-safe manner/serially,
     * otherwise the connection counter value will not be maintained accurately
     */
    virtual bool allowNewConnection() override;

    // ACCESSORS
    /**
     * \return Information about limiter as a string
     */
    virtual std::string toString() const override;

    /**
     * \return the connection limit (Allowed connections in particular time
     * window)
     */
    uint32_t getConnectionLimit() const;

    /**
     * \return the time window against which connections limit will be
     * calculated.
     */
    uint32_t getTimeWindowInSec() const;
};

}
}

#endif
