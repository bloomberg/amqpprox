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
#include <amqpprox_connectionstats.h>

#include <iostream>

#include <algorithm>

namespace Bloomberg {
namespace amqpprox {

const std::vector<std::string> ConnectionStats::s_sessionMetrics = {
    "packetsReceived",
    "packetsSent",
    "framesReceived",
    "framesSent",
    "bytesReceived",
    "bytesSent"};

const std::vector<std::string> ConnectionStats::s_statsTypes = {
    "pausedConnectionCount",
    "activeConnectionCount",
    "removedConnectionGraceful",
    "removedConnectionBrokerSnapped",
    "removedConnectionClientSnapped",
    "packetsReceived",
    "packetsSent",
    "framesReceived",
    "framesSent",
    "bytesReceived",
    "bytesSent"};

const std::vector<std::string> ConnectionStats::s_distributionMetrics = {
    "sendLatency",
    "receiveLatency"};

ConnectionStats::ConnectionStats()
: d_statsMap()
, d_distributionStatsMap()
{
    for (auto &statName : statsTypes()) {
        d_statsMap[statName] = 0;
    }

    for (auto &statName : distributionMetrics()) {
        d_distributionStatsMap[statName] =
            std::make_pair<uint64_t, uint64_t>(0, 0);
    }
}

ConnectionStats::ConnectionStats(
    const std::map<std::string, uint64_t> &stats,
    const std::map<std::string, std::pair<uint64_t, uint64_t>>
        &distributionStats)
: d_statsMap(stats)
, d_distributionStatsMap(distributionStats)
{
}

void ConnectionStats::swap(ConnectionStats &rhs)
{
    std::swap(d_statsMap, rhs.d_statsMap);
    std::swap(d_distributionStatsMap, rhs.d_distributionStatsMap);
}

void ConnectionStats::addDistributionStats(const std::string &name,
                                           uint64_t           total,
                                           uint64_t           count)
{
    d_distributionStatsMap[name].first += total;
    d_distributionStatsMap[name].second += count;
}

uint64_t ConnectionStats::distributionCount(const std::string &name) const
{
    auto it = d_distributionStatsMap.find(name);
    if (it == d_distributionStatsMap.end()) {
        return 0;
    }

    return it->second.second;
}

double ConnectionStats::distributionValue(const std::string &name) const
{
    auto it = d_distributionStatsMap.find(name);
    if (it == d_distributionStatsMap.end()) {
        return 0;
    }

    if (it->second.second == 0) {
        return 0;
    }

    return it->second.first / it->second.second;
}

std::pair<uint64_t, uint64_t>
ConnectionStats::distributionPair(const std::string &name) const
{
    const auto &it = d_distributionStatsMap.find(name);
    if (it == d_distributionStatsMap.end()) {
        return {0, 0};
    }
    return it->second;
}

bool ConnectionStats::operator==(const ConnectionStats &other) const
{
    return d_statsMap == other.d_statsMap &&
           d_distributionStatsMap == other.d_distributionStatsMap;
}

bool ConnectionStats::operator!=(const ConnectionStats &other) const
{
    return !(*this == other);
}

}
}
