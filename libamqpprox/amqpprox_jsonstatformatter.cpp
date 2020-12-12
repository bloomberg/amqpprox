/*
** Copyright 2020 Bloomberg Finance L.P.
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
#include <amqpprox_jsonstatformatter.h>

#include <iostream>
#include <string>

namespace Bloomberg {
namespace amqpprox {

void JsonStatFormatter::format(std::ostream &os, const ConnectionStats &stats)
{
    os << "{"
       << "\"pausedConnectionCount\": "
       << stats.statsValue("pausedConnectionCount") << ", "
       << "\"activeConnectionCount\": "
       << stats.statsValue("activeConnectionCount") << ", "
       << "\"removedConnectionGraceful\": "
       << stats.statsValue("removedConnectionGraceful") << ", "
       << "\"removedConnectionBrokerSnapped\": "
       << stats.statsValue("removedConnectionBrokerSnapped") << ", "
       << "\"removedConnectionClientSnapped\": "
       << stats.statsValue("removedConnectionClientSnapped") << ", "
       << "\"packetsReceived\": " << stats.statsValue("packetsReceived")
       << ", "
       << "\"packetsSent\": " << stats.statsValue("packetsSent") << ", "
       << "\"framesReceived\": " << stats.statsValue("framesReceived") << ", "
       << "\"framesSent\": " << stats.statsValue("framesSent") << ", "
       << "\"bytesReceived\": " << stats.statsValue("bytesReceived") << ", "
       << "\"bytesSent\": " << stats.statsValue("bytesSent") << "}";
}

void JsonStatFormatter::format(std::ostream &                os,
                               const StatSnapshot::StatsMap &statsMap)
{
    bool firstIteration = true;
    os << "{";
    for (const auto &stat : statsMap) {
        if (!firstIteration) {
            os << ", ";
        }
        else {
            firstIteration = false;
        }

        os << "\"" << stat.first << "\": ";
        format(os, stat.second);
    }
    os << "}";
}

void JsonStatFormatter::format(std::ostream &      os,
                               const StatSnapshot &statSnapshot)
{
    os << "{";
    os << "\"overall\": ";
    format(os, statSnapshot.overall());
    os << ", \"process\": ";
    format(os, statSnapshot.process());
    os << ", \"bufferpool\": ";
    format(os, statSnapshot.pool(), statSnapshot.poolSpillover());
    os << ", \"vhosts\": ";
    format(os, statSnapshot.vhosts());
    os << ", \"sources\": ";
    format(os, statSnapshot.sources());
    os << ", \"backends\": ";
    format(os, statSnapshot.backends());
    os << "}";
}

void JsonStatFormatter::format(std::ostream &                    os,
                               const StatSnapshot::ProcessStats &processStats)
{
    os << "{"
       << "\"cpu_percent_overall\": " << processStats.d_overall << ", "
       << "\"cpu_percent_user\": " << processStats.d_user << ", "
       << "\"cpu_percent_system\": " << processStats.d_system << ", "
       << "\"mem_rss_kb\": " << processStats.d_rssKB << "}";
}

void JsonStatFormatter::format(
    std::ostream &                              os,
    const std::vector<StatSnapshot::PoolStats> &poolStats,
    uint64_t                                    poolSpillover)
{
    os << "{"
       << "\"spill_to_heap_count\": " << poolSpillover << ", "
       << "\"pools\": {";

    bool firstIteration = true;
    for (const auto &pool : poolStats) {
        if (!firstIteration) {
            os << ", ";
        }
        else {
            firstIteration = false;
        }

        os << "\"" << pool.d_bufferSize
           << "\": { \"current\": " << pool.d_currentAllocation
           << ", \"highest\": " << pool.d_highwaterMark << "}";
    }

    os << "}}";
}

}
}
