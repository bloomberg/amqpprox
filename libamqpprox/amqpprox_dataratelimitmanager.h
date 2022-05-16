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
#ifndef BLOOMBERG_AMQPPROX_DATARATELIMITMANAGER
#define BLOOMBERG_AMQPPROX_DATARATELIMITMANAGER

#include <mutex>
#include <string>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

class DataRateLimitManager {
    std::unordered_map<std::string, std::size_t> d_vhostDataRateQuota;
    std::unordered_map<std::string, std::size_t> d_vhostDataRateAlarmQuota;
    std::size_t                                  d_defaultDataRateQuota;
    std::size_t                                  d_defaultDataRateAlarmQuota;
    mutable std::mutex                           d_mutex;

  public:
    DataRateLimitManager();

    /**
     * Get the rate limit for a particular vhost
     */
    std::size_t getDataRateLimit(const std::string &vhostName) const;

    /**
     * Get the rate alarm threshold for a particular vhost
     */
    std::size_t getDataRateAlarm(const std::string &vhostName) const;

    /**
     * Get the non-vhost-specific rate limit
     */
    std::size_t getDefaultDataRateLimit() const;

    /**
     * Get the non-vhost-specific alarm threshold
     */
    std::size_t getDefaultDataRateAlarm() const;

    /**
     * Set the non-vhost-specific rate limit
     */
    void setDefaultDataRateLimit(std::size_t quota);

    /**
     * Set the non-vhost-specific alarm threshold
     */
    void setDefaultDataRateAlarm(std::size_t quota);

    /**
     * Set the rate limit for a particular vhost
     */
    void setVhostDataRateLimit(const std::string &vhostName,
                               std::size_t        quota);

    /**
     * Set the rate alarm threshold for a particular vhost
     */
    void setVhostDataRateAlarm(const std::string &vhostName,
                               std::size_t        quota);

    /**
     * Disable the vhost specific rate limit
     */
    void disableVhostDataRateLimit(const std::string &vhostName);

    /**
     * Disable the vhost specific rate alarm threshold
     */
    void disableVhostDataRateAlarm(const std::string &vhostName);
};

}
}

#endif
