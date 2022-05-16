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
#ifndef BLOOMBERG_AMQPPROX_DATARATELIMIT
#define BLOOMBERG_AMQPPROX_DATARATELIMIT

#include <atomic>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Measures and reports usage with the aim to limit usage.
 *
 * \note Thread Safety - Calls to onTimer, recordUsage, and remainingQuota must
 * occur serially.
 *
 * \note Thread Safety - setQuota is safe from any thread
 */
class DataRateLimit {
  public:
    DataRateLimit();
    DataRateLimit(DataRateLimit &src);

    /**
     * Set the total permitted usage in bytes per second
     * \note std::numeric_limits<size_t>::max counts as infinite quota
     */
    void setQuota(std::size_t bytesPerSecond);

    /**
     * Fetch the currently configured quota
     */
    std::size_t getQuota() const;

    /**
     * Record some usage, which will affect the return of `remainingQuota`
     */
    void recordUsage(std::size_t bytesRead);

    /**
     * Query how much quota is left in the current time period
     */
    std::size_t remainingQuota() const;

    /**
     * Reset the current usage by incrementing the time period
     */
    void onTimer();

  private:
    std::atomic<std::size_t> d_quota;
    std::size_t              d_remainingQuota;
};

}
}

#endif
