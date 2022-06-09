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

#include <amqpprox_fixedwindowconnectionratelimiter.h>

#include <amqpprox_connectionlimiterinterface.h>

#include <chrono>
#include <memory>
#include <sstream>

namespace Bloomberg {
namespace amqpprox {

namespace {
const int SEC_TO_MS = 1000;
}

FixedWindowConnectionRateLimiter::FixedWindowConnectionRateLimiter(
    const std::shared_ptr<LimiterClock> &clockPtr,
    uint32_t                             connectionLimit,
    uint32_t                             timeWindowInSec)
: ConnectionLimiterInterface()
, d_clockPtr(clockPtr)
, d_connectionLimit(connectionLimit)
, d_timeWindowInMs(std::chrono::milliseconds(timeWindowInSec * SEC_TO_MS))
, d_lastTime(d_clockPtr->now())
, d_currentCount(0)
{
}

FixedWindowConnectionRateLimiter::FixedWindowConnectionRateLimiter(
    uint32_t connectionLimit,
    uint32_t timeWindowInSec)
: ConnectionLimiterInterface()
, d_clockPtr(std::make_shared<LimiterClock>())
, d_connectionLimit(connectionLimit)
, d_timeWindowInMs(std::chrono::milliseconds(timeWindowInSec * SEC_TO_MS))
, d_lastTime(d_clockPtr->now())
, d_currentCount(0)
{
}

bool FixedWindowConnectionRateLimiter::allowNewConnection()
{
    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::milliseconds>
        currentTime = d_clockPtr->now();

    // Reset the parameters for next time window
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - d_lastTime) >= d_timeWindowInMs) {
        d_lastTime     = currentTime;
        d_currentCount = 0;
    }

    if (d_currentCount >= d_connectionLimit)
        return false;

    d_currentCount += 1;
    return true;
}

std::string FixedWindowConnectionRateLimiter::toString() const
{
    std::stringstream ss;
    ss << "Allow average " << d_connectionLimit
       << " number of connections per " << d_timeWindowInMs.count() / SEC_TO_MS
       << " seconds";

    return ss.str();
}

uint32_t FixedWindowConnectionRateLimiter::getConnectionLimit() const
{
    return d_connectionLimit;
}

uint32_t FixedWindowConnectionRateLimiter::getTimeWindowInSec() const
{
    return d_timeWindowInMs.count() / SEC_TO_MS;
}

}
}
