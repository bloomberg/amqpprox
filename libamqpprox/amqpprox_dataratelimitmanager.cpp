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

#include <amqpprox_dataratelimitmanager.h>

#include <limits>
#include <mutex>
#include <string>

namespace Bloomberg {
namespace amqpprox {

DataRateLimitManager::DataRateLimitManager()
: d_vhostDataRateQuota()
, d_vhostDataRateAlarmQuota()
, d_defaultDataRateQuota(std::numeric_limits<std::size_t>::max())
, d_defaultDataRateAlarmQuota(std::numeric_limits<std::size_t>::max())
, d_mutex()
{
}

std::size_t
DataRateLimitManager::getDataRateLimit(const std::string &vhostName) const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    auto vhostRate = d_vhostDataRateQuota.find(vhostName);
    if (vhostRate == d_vhostDataRateQuota.end()) {
        return d_defaultDataRateQuota;
    }

    return vhostRate->second;
}

std::size_t
DataRateLimitManager::getDataRateAlarm(const std::string &vhostName) const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    auto vhostRate = d_vhostDataRateAlarmQuota.find(vhostName);
    if (vhostRate == d_vhostDataRateAlarmQuota.end()) {
        return d_defaultDataRateAlarmQuota;
    }

    return vhostRate->second;
}

std::size_t DataRateLimitManager::getDefaultDataRateLimit() const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    return d_defaultDataRateQuota;
}

std::size_t DataRateLimitManager::getDefaultDataRateAlarm() const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    return d_defaultDataRateAlarmQuota;
}

void DataRateLimitManager::setDefaultDataRateLimit(std::size_t quota)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultDataRateQuota = quota;
}

void DataRateLimitManager::setDefaultDataRateAlarm(std::size_t quota)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultDataRateAlarmQuota = quota;
}

void DataRateLimitManager::setVhostDataRateLimit(const std::string &vhostName,
                                                 std::size_t        quota)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_vhostDataRateQuota[vhostName] = quota;
}

void DataRateLimitManager::setVhostDataRateAlarm(const std::string &vhostName,
                                                 std::size_t        quota)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_vhostDataRateAlarmQuota[vhostName] = quota;
}

void DataRateLimitManager::disableVhostDataRateLimit(
    const std::string &vhostName)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_vhostDataRateQuota.erase(vhostName);
}
void DataRateLimitManager::disableVhostDataRateAlarm(
    const std::string &vhostName)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_vhostDataRateAlarmQuota.erase(vhostName);
}

}
}
