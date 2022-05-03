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

#include <amqpprox_fixedwindowconnectionratelimiter.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <string>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

namespace {
struct MockLimiterClock : public LimiterClock {
    virtual ~MockLimiterClock() override = default;
    MOCK_METHOD0(now,
                 std::chrono::time_point<std::chrono::steady_clock,
                                         std::chrono::milliseconds>());
};

class MockFixedWindowConnectionRateLimiter
: public FixedWindowConnectionRateLimiter {
  public:
    MockFixedWindowConnectionRateLimiter(
        const std::shared_ptr<LimiterClock> &clockPtr,
        uint32_t                             connectionLimit,
        uint32_t                             timeWindowInSec = 1)
    : FixedWindowConnectionRateLimiter(clockPtr,
                                       connectionLimit,
                                       timeWindowInSec)
    {
    }

    virtual ~MockFixedWindowConnectionRateLimiter() override = default;
};

}

TEST(FixedWindowConnectionRateLimiterTest, Breathing)
{
    uint32_t                         connectionLimit = 1000;
    FixedWindowConnectionRateLimiter rateLimiter(connectionLimit);
    EXPECT_EQ(rateLimiter.getConnectionLimit(), 1000);
    EXPECT_EQ(rateLimiter.getTimeWindowInSec(), 1);
}

TEST(FixedWindowConnectionRateLimiterTest, ToString)
{
    uint32_t                         connectionLimit = 1000;
    uint32_t                         timeWindowInSec = 10;
    FixedWindowConnectionRateLimiter rateLimiter(connectionLimit,
                                                 timeWindowInSec);
    EXPECT_EQ(rateLimiter.toString(),
              "Allow average " + std::to_string(connectionLimit) +
                  " number of connections per " +
                  std::to_string(timeWindowInSec) + " seconds");
}

TEST(FixedWindowConnectionRateLimiterTest, AllowNewConnection)
{
    using namespace std::chrono_literals;
    uint32_t connectionLimit = 1;

    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::milliseconds>
        currentTime = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now());

    std::shared_ptr<MockLimiterClock> mockClockPtr =
        std::make_shared<MockLimiterClock>();

    EXPECT_CALL(*mockClockPtr, now())
        .Times(4)
        // Called during FixedWindowConnectionRateLimiter initialization
        .WillOnce(Return(currentTime))
        // Called inside allowNewConnection method first call
        .WillOnce(Return(currentTime))
        // Called inside allowNewConnection method second call
        .WillOnce(Return(currentTime + 500ms))
        // Called inside allowNewConnection method third call
        .WillOnce(Return(currentTime + 1000ms));

    MockFixedWindowConnectionRateLimiter rateLimiter(mockClockPtr,
                                                     connectionLimit);
    // Called at 0 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());

    // Called after 500 milliseconds
    EXPECT_FALSE(rateLimiter.allowNewConnection());

    // Called after 1000 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());
}

TEST(FixedWindowConnectionRateLimiterTest, AllowNewConnectionOverlap)
{
    using namespace std::chrono_literals;
    uint32_t connectionLimit = 2;

    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::milliseconds>
        currentTime = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now());

    std::shared_ptr<MockLimiterClock> mockClockPtr =
        std::make_shared<MockLimiterClock>();
    EXPECT_CALL(*mockClockPtr, now())
        .Times(6)
        // Called during FixedWindowConnectionRateLimiter initialization
        .WillOnce(Return(currentTime))
        // Called inside allowNewConnection method first call
        .WillOnce(Return(currentTime + 800ms))
        // Called inside allowNewConnection method second call
        .WillOnce(Return(currentTime + 900ms))
        // Called inside allowNewConnection method third call
        .WillOnce(Return(currentTime + 1000ms))
        // Called inside allowNewConnection method fourth call
        .WillOnce(Return(currentTime + 1100ms))
        // Called inside allowNewConnection method fifth call
        .WillOnce(Return(currentTime + 1500ms));

    MockFixedWindowConnectionRateLimiter rateLimiter(mockClockPtr,
                                                     connectionLimit);
    // Called after 800 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());

    // Called after 900 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());

    // Called after 1000 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());

    // Called after 1100 milliseconds
    EXPECT_TRUE(rateLimiter.allowNewConnection());

    // Called after 1500 milliseconds
    EXPECT_FALSE(rateLimiter.allowNewConnection());
}
