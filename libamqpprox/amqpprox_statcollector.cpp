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
#include <amqpprox_statcollector.h>

#include <amqpprox_bufferpool.h>
#include <amqpprox_cpumonitor.h>
#include <amqpprox_sessionstate.h>

#include <boost/lexical_cast.hpp>

namespace Bloomberg {
namespace amqpprox {

StatCollector::StatCollector()
: d_current()
, d_previous()
, d_cpuMonitor_p(nullptr)
, d_bufferPool_p(nullptr)
{
}

void StatCollector::reset()
{
    d_previous.swap(d_current);
    StatSnapshot temp;
    d_current.swap(temp);
}

void StatCollector::setCpuMonitor(CpuMonitor *monitor)
{
    d_cpuMonitor_p = monitor;
}

void StatCollector::setBufferPool(BufferPool *pool)
{
    d_bufferPool_p = pool;
}

void StatCollector::collect(const SessionState &session)
{
    auto vhost = session.getVirtualHost();

    // For backends we care about the broker's port to disambiguate
    // the different brokers running on a host.  Note that we are
    // using '_' as the separator, as ':' is not compatible with
    // statsD.
    auto backend =
        session.hostname(session.getEgress().second) + "_" +
        boost::lexical_cast<std::string>(session.getEgress().second.port());

    // For sources we only look at the machine, not the ephemeral port
    auto source = session.hostname(session.getIngress().second);

    auto &vhostStats   = d_current.vhosts()[vhost];
    auto &backendStats = d_current.backends()[backend];
    auto &sourceStats  = d_current.sources()[source];

    uint64_t ingressPackets, ingressFrames, ingressBytes, ingressLatencyCount,
        ingressLatencyTotal, egressPackets, egressFrames, egressBytes,
        egressLatencyCount, egressLatencyTotal;

    session.getTotals(&ingressPackets,
                      &ingressFrames,
                      &ingressBytes,
                      &ingressLatencyTotal,
                      &ingressLatencyCount,
                      &egressPackets,
                      &egressFrames,
                      &egressBytes,
                      &egressLatencyTotal,
                      &egressLatencyCount);

    using DiscoType  = SessionState::DisconnectType;
    auto discoStatus = session.getDisconnectType();

    auto addStats = [&egressPackets,
                     &ingressPackets,
                     &egressFrames,
                     &ingressFrames,
                     &egressBytes,
                     &ingressBytes,
                     &egressLatencyTotal,
                     &ingressLatencyTotal,
                     &egressLatencyCount,
                     &ingressLatencyCount,
                     &session,
                     &discoStatus](ConnectionStats &statsObject) {
        statsObject.statsValue("packetsSent") += egressPackets;
        statsObject.statsValue("packetsReceived") += ingressPackets;
        statsObject.statsValue("framesSent") += egressFrames;
        statsObject.statsValue("framesReceived") += ingressFrames;
        statsObject.statsValue("bytesSent") += egressBytes;
        statsObject.statsValue("bytesReceived") += ingressBytes;
        statsObject.addDistributionStats(
            "sendLatency", egressLatencyTotal, egressLatencyCount);
        statsObject.addDistributionStats(
            "receiveLatency", ingressLatencyTotal, ingressLatencyCount);

        if (discoStatus == DiscoType::DISCONNECTED_CLIENT) {
            statsObject.statsValue("removedConnectionClientSnapped") += 1;
        }
        else if (discoStatus == DiscoType::DISCONNECTED_SERVER) {
            statsObject.statsValue("removedConnectionBrokerSnapped") += 1;
        }
        else if (discoStatus == DiscoType::DISCONNECTED_CLEANLY ||
                 discoStatus == DiscoType::DISCONNECTED_PROXY) {
            statsObject.statsValue("removedConnectionGraceful") += 1;
        }
        else {
            statsObject.statsValue("activeConnectionCount") += 1;
        }

        // We count paused separately
        if (session.getPaused()) {
            statsObject.statsValue("pausedConnectionCount") += 1;
        }
    };

    addStats(vhostStats);
    addStats(backendStats);
    addStats(sourceStats);
    addStats(d_current.overall());
}

void StatCollector::deletedSession(const SessionState &session)
{
    // Implementation notes:
    //
    // This method is required because the accumulated statistics are not kept
    // per session in this component, and only accumulated across various axes.
    // Each session object contains a counter of total bytes, packets and
    // frames for each direction, because these are ever increasing totals,
    // we latch the values and compare them in this class versus the last
    // collected values. A-B-A type issues arise when the only session for one
    // of the axes is deleted in one collection cycle, which must have its
    // values counted, then it is reestablished in the subsequent cycle under a
    // new session. This leaves a vhost/backend/source with a large count on
    // the d_previous snapshot, and a small count on the d_current. These
    // cannot be compared, so instead before resetting the accumulation
    // interval, but after retrieving any statistics, we remove the
    // accumulation totals for all the deleted sessions again in this method.

    auto vhost = session.getVirtualHost();

    // For backends we care about the broker's port to disambiguate
    // the different brokers running on a host.  Note that we are
    // using '_' as the separator, as ':' is not compatible with
    // statsD.
    auto backend =
        session.hostname(session.getEgress().second) + "_" +
        boost::lexical_cast<std::string>(session.getEgress().second.port());

    // For sources we only look at the machine, not the ephemeral port
    auto source = session.hostname(session.getIngress().second);

    auto &vhostStats   = d_current.vhosts()[vhost];
    auto &backendStats = d_current.backends()[backend];
    auto &sourceStats  = d_current.sources()[source];

    uint64_t ingressPackets, ingressFrames, ingressBytes, ingressLatencyCount,
        ingressLatencyTotal, egressPackets, egressFrames, egressBytes,
        egressLatencyCount, egressLatencyTotal;

    session.getTotals(&ingressPackets,
                      &ingressFrames,
                      &ingressBytes,
                      &ingressLatencyTotal,
                      &ingressLatencyCount,
                      &egressPackets,
                      &egressFrames,
                      &egressBytes,
                      &egressLatencyTotal,
                      &egressLatencyCount);

    auto remStats = [&egressPackets,
                     &ingressPackets,
                     &egressFrames,
                     &ingressFrames,
                     &egressBytes,
                     &ingressBytes,
                     &egressLatencyTotal,
                     &ingressLatencyTotal,
                     &egressLatencyCount,
                     &ingressLatencyCount](ConnectionStats &statsObject) {
        statsObject.statsValue("packetsSent") -= egressPackets;
        statsObject.statsValue("packetsReceived") -= ingressPackets;
        statsObject.statsValue("framesSent") -= egressFrames;
        statsObject.statsValue("framesReceived") -= ingressFrames;
        statsObject.statsValue("bytesSent") -= egressBytes;
        statsObject.statsValue("bytesReceived") -= ingressBytes;
        statsObject.addDistributionStats(
            "sendLatency", -egressLatencyTotal, -egressLatencyCount);
        statsObject.addDistributionStats(
            "receiveLatency", -ingressLatencyTotal, -ingressLatencyCount);
    };

    remStats(vhostStats);
    remStats(backendStats);
    remStats(sourceStats);
    remStats(d_current.overall());
}

void StatCollector::populateStats(StatSnapshot *snap)
{
    populateMap(&snap->sources(), d_current.sources(), d_previous.sources());
    populateMap(&snap->vhosts(), d_current.vhosts(), d_previous.vhosts());
    populateMap(
        &snap->backends(), d_current.backends(), d_previous.backends());
    populateProgramStats(&snap->overall());

    if (d_cpuMonitor_p && d_cpuMonitor_p->valid()) {
        auto &pstats     = snap->process();
        auto  cpustats   = d_cpuMonitor_p->currentCpu();
        pstats.d_rssKB   = d_cpuMonitor_p->currentRssKB();
        pstats.d_user    = std::round(std::get<0>(cpustats) * 100.0);
        pstats.d_system  = std::round(std::get<1>(cpustats) * 100.0);
        pstats.d_overall = std::round(
            (std::get<0>(cpustats) + std::get<1>(cpustats)) * 100.0);
    }

    if (d_bufferPool_p) {
        std::vector<BufferPool::BufferAllocationStat> poolstats;
        uint64_t                                      poolSpillover;
        d_bufferPool_p->getPoolStatistics(&poolstats, &poolSpillover);

        snap->poolSpillover() = poolSpillover;
        for (const auto &ps : poolstats) {
            StatSnapshot::PoolStats outputStats;
            outputStats.d_bufferSize        = std::get<0>(ps);
            outputStats.d_currentAllocation = std::get<1>(ps);
            outputStats.d_highwaterMark     = std::get<2>(ps);
            snap->pool().push_back(outputStats);
        }
    }
}

void StatCollector::populateProgramStats(ConnectionStats *programStats) const
{
    ConnectionStats val;
    val             = d_current.overall();
    auto &prevStats = d_previous.overall();

    for (auto &name : ConnectionStats::sessionMetrics()) {
        val.statsValue(name) -= prevStats.statsValue(name);
    }

    for (auto &name : ConnectionStats::distributionMetrics()) {
        const auto &prevDistribution = prevStats.distributionPair(name);
        val.addDistributionStats(
            name, -prevDistribution.first, -prevDistribution.second);
    }

    *programStats = val;
}

void StatCollector::populateMap(StatSnapshot::StatsMap *      map,
                                const StatSnapshot::StatsMap &source,
                                const StatSnapshot::StatsMap &previous) const
{
    StatSnapshot::StatsMap output(source);
    ConnectionStats        zeroStats;

    for (auto &k : output) {
        const ConnectionStats *prevValue = &zeroStats;

        auto it = previous.find(k.first);
        if (it != previous.end()) {
            prevValue = &it->second;
        }

        auto &val = k.second;
        for (auto &name : ConnectionStats::sessionMetrics()) {
            val.statsValue(name) -= prevValue->statsValue(name);
        }

        for (auto &name : ConnectionStats::distributionMetrics()) {
            const auto &prevDistribution = prevValue->distributionPair(name);
            val.addDistributionStats(
                name, -prevDistribution.first, -prevDistribution.second);
        }
    }

    map->swap(output);
}

}
}
