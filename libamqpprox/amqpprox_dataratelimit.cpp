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

#include <amqpprox_dataratelimit.h>

#include <algorithm>
#include <limits>

namespace Bloomberg {
namespace amqpprox {

DataRateLimit::DataRateLimit()
: d_quota(std::numeric_limits<std::size_t>::max())
, d_remainingQuota(d_quota)
{
}

DataRateLimit::DataRateLimit(DataRateLimit &src)
: d_quota(src.d_quota.load())
, d_remainingQuota(src.d_remainingQuota)
{
}

void DataRateLimit::setQuota(std::size_t bytesPerSecond)
{
    d_quota = bytesPerSecond;
}

std::size_t DataRateLimit::getQuota() const
{
    return d_quota;
}

void DataRateLimit::recordUsage(std::size_t bytesRead)
{
    if (d_quota == std::numeric_limits<std::size_t>::max()) {
        // We don't trigger quota limits for this d_quota value
        return;
    }

    if (d_quota < d_remainingQuota) {
        // Apply the updated, restricted quota without needing to synchronise
        // setQuota and recordUsage
        d_remainingQuota = d_quota;
    }

    d_remainingQuota -= std::min(d_remainingQuota, bytesRead);
}

std::size_t DataRateLimit::remainingQuota() const
{
    return d_remainingQuota;
}

void DataRateLimit::onTimer()
{
    d_remainingQuota = d_quota;
}

}
}
