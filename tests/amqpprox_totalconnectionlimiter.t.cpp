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

#include <amqpprox_totalconnectionlimiter.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(TotalConnectionLimiterTest, Breathing)
{
    uint32_t               totalConnectionLimit = 1000;
    TotalConnectionLimiter limiter(totalConnectionLimit);
    EXPECT_EQ(limiter.getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter.getConnectionCount(), 0);
}

TEST(TotalConnectionLimiterTest, ToString)
{
    uint32_t               totalConnectionLimit = 1000;
    TotalConnectionLimiter limiter(totalConnectionLimit);
    EXPECT_EQ(limiter.toString(),
              "Allow total " + std::to_string(totalConnectionLimit) +
                  " connections");
}

TEST(TotalConnectionLimiterTest, AllowNewConnectionAndCloseConnection)
{
    uint32_t               totalConnectionLimit = 1;
    TotalConnectionLimiter limiter(totalConnectionLimit);
    EXPECT_EQ(limiter.getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter.getConnectionCount(), 0);

    EXPECT_TRUE(limiter.allowNewConnection());
    EXPECT_FALSE(limiter.allowNewConnection());
    EXPECT_EQ(limiter.getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter.getConnectionCount(), totalConnectionLimit);

    limiter.connectionClosed();
    EXPECT_EQ(limiter.getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter.getConnectionCount(), 0);

    EXPECT_TRUE(limiter.allowNewConnection());
    EXPECT_FALSE(limiter.allowNewConnection());
    EXPECT_EQ(limiter.getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter.getConnectionCount(), totalConnectionLimit);
}
