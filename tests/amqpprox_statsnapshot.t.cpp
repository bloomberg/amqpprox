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
#include <amqpprox_statsnapshot.h>

#include <gtest/gtest.h>

using namespace Bloomberg::amqpprox;

TEST(StatSnapshot, Breathing)
{
    StatSnapshot               snapshot;
    ConnectionStats            emptyStats;
    StatSnapshot::ProcessStats emptyProcessStats;

    EXPECT_EQ(snapshot.overall(), emptyStats);
    EXPECT_TRUE(snapshot.vhosts().empty());
    EXPECT_TRUE(snapshot.sources().empty());
    EXPECT_TRUE(snapshot.backends().empty());
    EXPECT_TRUE(snapshot.pool().empty());
    EXPECT_EQ(snapshot.process(), emptyProcessStats);
    EXPECT_EQ(snapshot.poolSpillover(), 0);
}

TEST(StatSnapshot, Swap)
{
    StatSnapshot::ProcessStats emptyProcessStats;
    StatSnapshot::PoolStats    emptyPoolStats;

    ConnectionStats stats1;
    int             i = 1;
    for (auto &name : ConnectionStats::statsTypes()) {
        stats1.statsValue(name) = i++;
    }

    StatSnapshot snapshot1, snapshot2;

    snapshot1.overall()         = stats1;
    snapshot1.vhosts()["foo"]   = stats1;
    snapshot1.backends()["bar"] = stats1;
    snapshot1.sources()["baz"]  = stats1;
    snapshot1.process().d_user  = 1;
    snapshot1.pool().push_back(emptyPoolStats);
    snapshot1.poolSpillover() = 2;

    snapshot1.swap(snapshot2);

    EXPECT_EQ(snapshot1.overall(), ConnectionStats());
    EXPECT_TRUE(snapshot1.vhosts().empty());
    EXPECT_TRUE(snapshot1.backends().empty());
    EXPECT_TRUE(snapshot1.sources().empty());
    EXPECT_EQ(snapshot1.process(), emptyProcessStats);
    EXPECT_TRUE(snapshot1.pool().empty());
    EXPECT_EQ(snapshot1.poolSpillover(), 0);

    EXPECT_EQ(snapshot2.overall(), stats1);
    EXPECT_EQ(snapshot2.vhosts()["foo"], stats1);
    EXPECT_EQ(snapshot2.backends()["bar"], stats1);
    EXPECT_EQ(snapshot2.sources()["baz"], stats1);
    EXPECT_EQ(snapshot2.process().d_user, 1);
    EXPECT_FALSE(snapshot2.pool().empty());
    EXPECT_EQ(snapshot2.poolSpillover(), 2);
}
