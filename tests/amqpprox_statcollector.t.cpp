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

#include <map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(StatCollector, Breathing)
{
    StatCollector sc;
}

TEST(StatCollector, Basic_Reset)
{
    StatCollector sc;

    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), zeroStats);

    snapshot = StatSnapshot();

    // Make sure this holds at zero after a reset
    sc.reset();

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), zeroStats);
}

TEST(StatCollector, Zeros)
{
    StatCollector sc;

    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), zeroStats);
    EXPECT_TRUE(snapshot.vhosts().empty());
    EXPECT_TRUE(snapshot.sources().empty());
    EXPECT_TRUE(snapshot.backends().empty());
}

TEST(StatCollector, Simple_Single_Session)
{
    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    SessionState state(nullptr);
    state.setVirtualHost("foo");
    state.incrementEgressTotals(2, 3);
    state.incrementIngressTotals(4, 5);

    StatCollector sc;
    sc.collect(state);

    ConnectionStats expectedStats(
        {{"pausedConnectionCount", 0},
         {"activeConnectionCount", 1},
         {"authDeniedConnectionCount", 0},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 1},
         {"packetsSent", 1},
         {"framesReceived", 4},
         {"framesSent", 2},
         {"bytesReceived", 5},
         {"bytesSent", 3}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {0, 0}}});

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), expectedStats);

    sc.deletedSession(state);
    sc.reset();
    // No collect() calls

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), zeroStats);
}

TEST(StatCollector, Multiple_Session)
{
    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    SessionState state1(nullptr);
    SessionState state2(nullptr);
    SessionState state3(nullptr);
    state1.setVirtualHost("foo");
    state1.incrementEgressTotals(2, 3);
    state1.incrementIngressTotals(4, 5);
    state1.addIngressLatency(4);
    state2.setVirtualHost("foo");
    state2.incrementEgressTotals(10, 20);
    state2.incrementIngressTotals(30, 40);
    state2.addIngressLatency(5);
    state2.addIngressLatency(6);
    state3.setVirtualHost("bar");
    state3.incrementEgressTotals(100, 200);
    state3.incrementIngressTotals(300, 400);
    state3.incrementIngressTotals(3000, 4000);
    state3.addIngressLatency(2);
    state3.setPaused(true);
    state3.setAuthDeniedConnection(true);

    StatCollector sc;
    sc.collect(state1);
    sc.collect(state2);
    sc.collect(state3);

    ConnectionStats expectedStats(
        {{"pausedConnectionCount", 1},
         {"activeConnectionCount", 3},
         {"authDeniedConnectionCount", 1},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 4},
         {"packetsSent", 3},
         {"framesReceived", 4 + 30 + 300 + 3000},
         {"framesSent", 2 + 10 + 100},
         {"bytesReceived", 5 + 40 + 400 + 4000},
         {"bytesSent", 3 + 20 + 200}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {4 + 5 + 6 + 2, 4}}});

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), expectedStats);

    ConnectionStats expectedFooStats(
        {{"pausedConnectionCount", 0},
         {"activeConnectionCount", 2},
         {"authDeniedConnectionCount", 0},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 2},
         {"packetsSent", 2},
         {"framesReceived", 4 + 30},
         {"framesSent", 2 + 10},
         {"bytesReceived", 5 + 40},
         {"bytesSent", 3 + 20}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {4 + 5 + 6, 3}}});

    ConnectionStats expectedBarStats(
        {{"pausedConnectionCount", 1},
         {"activeConnectionCount", 1},
         {"authDeniedConnectionCount", 1},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 2},
         {"packetsSent", 1},
         {"framesReceived", 300 + 3000},
         {"framesSent", 100},
         {"bytesReceived", 400 + 4000},
         {"bytesSent", 200}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {2, 1}}});

    EXPECT_EQ(expectedFooStats, snapshot.vhosts()["foo"]);
    EXPECT_EQ(expectedBarStats, snapshot.vhosts()["bar"]);

    // Ensure resetting holds returning zeros afterwards
    sc.deletedSession(state1);
    sc.deletedSession(state2);
    sc.deletedSession(state3);
    sc.reset();
    // NB: no collect()

    snapshot = StatSnapshot();
    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), zeroStats);
}

TEST(StatCollector, Returns_To_Zero)
{
    // In this test we verify that the *same* state being collected in a newly
    // reset round correctly stays at zero for all the counter metrics like
    // bytes received.
    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    SessionState state(nullptr);
    state.setVirtualHost("foo");
    state.incrementEgressTotals(2, 3);
    state.incrementIngressTotals(4, 5);
    state.addIngressLatency(2);
    state.addEgressLatency(3);

    StatCollector sc;
    sc.collect(state);

    sc.populateStats(&snapshot);
    EXPECT_NE(snapshot.overall(), zeroStats);

    sc.reset();
    sc.collect(state);

    ConnectionStats expectedStats(
        {{"pausedConnectionCount", 0},
         {"activeConnectionCount", 1},
         {"authDeniedConnectionCount", 0},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 0},
         {"packetsSent", 0},
         {"framesReceived", 0},
         {"framesSent", 0},
         {"bytesReceived", 0},
         {"bytesSent", 0}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {0, 0}}});

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), expectedStats);
}

TEST(StatCollector, Handles_New_Counter_Values)
{
    // In this test we verify that state being collected with new counter
    // values in a newly reset round correctly accounts for them
    ConnectionStats zeroStats;
    StatSnapshot    snapshot;

    SessionState state(nullptr);
    state.setVirtualHost("foo");
    state.incrementEgressTotals(2, 3);
    state.incrementIngressTotals(4, 5);

    StatCollector sc;
    sc.collect(state);

    sc.populateStats(&snapshot);
    EXPECT_NE(snapshot.overall(), zeroStats);

    sc.reset();

    state.incrementEgressTotals(20, 30);
    state.incrementIngressTotals(40, 50);
    sc.collect(state);

    ConnectionStats expectedStats(
        {{"pausedConnectionCount", 0},
         {"activeConnectionCount", 1},
         {"authDeniedConnectionCount", 0},
         {"limitedConnectionCount", 0},
         {"removedConnectionGraceful", 0},
         {"removedConnectionBrokerSnapped", 0},
         {"removedConnectionClientSnapped", 0},
         {"packetsReceived", 1},
         {"packetsSent", 1},
         {"framesReceived", 40},
         {"framesSent", 20},
         {"bytesReceived", 50},
         {"bytesSent", 30}},
        {{"sendLatency", {0, 0}}, {"receiveLatency", {0, 0}}});

    sc.populateStats(&snapshot);
    EXPECT_EQ(snapshot.overall(), expectedStats);
}

TEST(StatCollector, Cpu_Monitor_Is_Picked_Up)
{
    CpuMonitor    cpuMonitor;
    StatCollector sc;
    sc.setCpuMonitor(&cpuMonitor);

    cpuMonitor.clock(nullptr, nullptr);
    cpuMonitor.clock(nullptr, nullptr);

    EXPECT_TRUE(cpuMonitor.valid());

    StatSnapshot stats;
    sc.populateStats(&stats);

    auto processStats = stats.process();
    EXPECT_GT(processStats.d_rssKB, 0);
    // We can't check the other CPU stats because there's not enough work
    // between samples.
}

TEST(StatCollector, Cpu_Monitor_Not_Enough_Samples)
{
    CpuMonitor    cpuMonitor;
    StatCollector sc;
    sc.setCpuMonitor(&cpuMonitor);

    EXPECT_FALSE(cpuMonitor.valid());

    StatSnapshot stats;
    sc.populateStats(&stats);

    auto processStats = stats.process();
    EXPECT_EQ(processStats.d_rssKB, 0);
    EXPECT_EQ(processStats.d_user, 0);
    EXPECT_EQ(processStats.d_system, 0);
    EXPECT_EQ(processStats.d_overall, 0);
}

TEST(StatCollector, Pool_Empty)
{
    BufferPool    bp({});
    StatCollector sc;
    sc.setBufferPool(&bp);

    StatSnapshot stats;
    sc.populateStats(&stats);

    EXPECT_TRUE(stats.pool().empty());
}

TEST(StatCollector, Pool_Two_Stats)
{
    BufferPool    bp({100, 200});
    StatCollector sc;
    sc.setBufferPool(&bp);

    BufferHandle handle;
    BufferHandle handle2;
    bp.acquireBuffer(&handle, 99);
    bp.acquireBuffer(&handle2, 99);
    handle.release();

    BufferHandle handle3;
    bp.acquireBuffer(&handle3, 101);
    bp.acquireBuffer(&handle3, 1000);

    StatSnapshot stats;
    sc.populateStats(&stats);

    EXPECT_EQ(stats.pool().size(), 2);
    EXPECT_EQ(stats.poolSpillover(), 1);

    std::map<std::size_t, StatSnapshot::PoolStats> statMap;
    auto                                           pools = stats.pool();
    for (const auto &pool : pools) {
        statMap[pool.d_bufferSize] = pool;
    }

    EXPECT_EQ(statMap[100].d_highwaterMark, 2);
    EXPECT_EQ(statMap[100].d_currentAllocation, 1);
    EXPECT_EQ(statMap[200].d_highwaterMark, 1);
    EXPECT_EQ(statMap[200].d_currentAllocation, 0);
}
