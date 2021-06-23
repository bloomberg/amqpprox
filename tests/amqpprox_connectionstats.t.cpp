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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(ConnectionStats, Breathing)
{
    ConnectionStats cs;
    for (auto &name : cs.statsTypes()) {
        EXPECT_EQ(cs.statsValue(name), 0);
    }
}

TEST(ConnectionStats, BasicEquality)
{
    ConnectionStats cs1, cs2;
    EXPECT_EQ(cs1, cs2);
    cs1.statsValue("pausedConnectionCount") += 1;
    cs2.statsValue("pausedConnectionCount") += 1;
    EXPECT_EQ(cs1, cs2);
    cs1.statsValue("pausedConnectionCount") += 1;
    EXPECT_NE(cs1, cs2);
}
