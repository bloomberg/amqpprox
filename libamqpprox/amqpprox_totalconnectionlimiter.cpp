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

#include <amqpprox_totalconnectionlimiter.h>

#include <sstream>

namespace Bloomberg {
namespace amqpprox {

TotalConnectionLimiter::TotalConnectionLimiter(uint32_t totalConnectionLimit)
: ConnectionLimiterInterface()
, d_totalConnectionLimit(totalConnectionLimit)
, d_connectionCount(0)
{
}

bool TotalConnectionLimiter::allowNewConnection()
{
    if (d_connectionCount < d_totalConnectionLimit) {
        d_connectionCount++;
        return true;
    }
    return false;
}

void TotalConnectionLimiter::connectionClosed()
{
    if (d_connectionCount == 0) {
        // This is possible when the limiter is set up, while having already
        // some active connections
        return;
    }

    d_connectionCount--;
}

std::string TotalConnectionLimiter::toString() const
{
    std::stringstream ss;
    ss << "Allow total " << d_totalConnectionLimit << " connections";

    return ss.str();
}

uint32_t TotalConnectionLimiter::getTotalConnectionLimit() const
{
    return d_totalConnectionLimit;
}

uint32_t TotalConnectionLimiter::getConnectionCount() const
{
    return d_connectionCount;
}

}
}
