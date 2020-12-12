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
#include <amqpprox_humanstatformatter.h>

#include <iostream>
#include <string>

namespace Bloomberg {
namespace amqpprox {

namespace {

void humanBytes(std::ostream &os, uint64_t bytes)
{
    constexpr uint64_t kb = 1024;
    constexpr uint64_t mb = kb * kb;
    constexpr uint64_t gb = mb * kb;
    constexpr uint64_t tb = gb * kb;

    if (bytes < kb) {
        os << bytes << " B";
    }
    else if (bytes < mb) {
        os << bytes / (1.0 * kb) << " KB";
    }
    else if (bytes < gb) {
        os << bytes / (1.0 * mb) << " MB";
    }
    else if (bytes < tb) {
        os << bytes / (1.0 * gb) << " GB";
    }
    else {
        os << bytes << " B";
    }
}

}

void HumanStatFormatter::format(std::ostream &os, const ConnectionStats &stats)
{
    os << "Paused: " << stats.statsValue("pausedConnectionCount") << " "
       << "Active: " << stats.statsValue("activeConnectionCount") << " "
       << "Removed(Clean): " << stats.statsValue("removedConnectionGraceful")
       << " "
       << "Removed(Broker): "
       << stats.statsValue("removedConnectionBrokerSnapped") << " "
       << "Removed(Client): "
       << stats.statsValue("removedConnectionClientSnapped") << " ";

    os << "IN: ";
    humanBytes(os, stats.statsValue("bytesReceived"));
    os << "/s " << stats.statsValue("packetsReceived") << " pkt/s "
       << stats.statsValue("framesReceived") << " frames/s ";

    os << "OUT: ";
    humanBytes(os, stats.statsValue("bytesSent"));
    os << "/s " << stats.statsValue("packetsSent") << " pkt/s "
       << stats.statsValue("framesSent") << " frames/s";
}

void HumanStatFormatter::format(std::ostream &                os,
                                const StatSnapshot::StatsMap &statsMap)
{
    for (const auto &stat : statsMap) {
        os << stat.first << ": ";
        format(os, stat.second);
        os << "\n";
    }
}

void HumanStatFormatter::format(std::ostream &      os,
                                const StatSnapshot &statSnapshot)
{
    os << "Overall:\n";
    format(os, statSnapshot.overall());
    os << "\n";
    os << "Process:\n";
    format(os, statSnapshot.process());
    os << "\n";
    os << "BufferPool:\n";
    format(os, statSnapshot.pool(), statSnapshot.poolSpillover());
    os << "\n";
    os << "Vhosts:\n";
    format(os, statSnapshot.vhosts());
    os << "Sources:\n";
    format(os, statSnapshot.sources());
    os << "Backends:\n";
    format(os, statSnapshot.backends());
}

void HumanStatFormatter::format(std::ostream &                    os,
                                const StatSnapshot::ProcessStats &processStats)
{
    os << "CPU%: " << processStats.d_overall << " "
       << "USR%: " << processStats.d_user << " "
       << "SYS%: " << processStats.d_system << " "
       << "RSS: ";
    humanBytes(os, processStats.d_rssKB * 1024);
}

void HumanStatFormatter::format(
    std::ostream &                              os,
    const std::vector<StatSnapshot::PoolStats> &poolStats,
    uint64_t                                    poolSpillover)
{
    os << "Spilt to heap: " << poolSpillover << ", Pools (Current/Peak): ";

    for (const auto &pool : poolStats) {
        if (&pool != &poolStats.front()) {
            os << ", ";
        }

        os << pool.d_bufferSize << "=" << pool.d_currentAllocation << "/"
           << pool.d_highwaterMark;
    }
}

}
}
