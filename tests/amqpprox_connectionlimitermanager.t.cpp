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

#include <amqpprox_connectionlimitermanager.h>

#include <amqpprox_fixedwindowconnectionratelimiter.h>
#include <amqpprox_totalconnectionlimiter.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(ConnectionLimiterManagerTest, Breathing)
{
    ConnectionLimiterManager limiterManager;
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.getAlarmOnlyConnectionRateLimiter(
                    "test-vhost") == nullptr);
    EXPECT_TRUE(limiterManager.getConnectionRateLimiter("test-vhost") ==
                nullptr);
    EXPECT_TRUE(limiterManager.getAlarmOnlyTotalConnectionLimiter(
                    "test-vhost") == nullptr);
    EXPECT_TRUE(limiterManager.getTotalConnectionLimiter("test-vhost") ==
                nullptr);
}

TEST(ConnectionLimiterManagerTest, AddGetRemoveConnectionRateLimiter)
{
    ConnectionLimiterManager limiterManager;

    std::string vhostName1       = "test-vhost1";
    std::string vhostName2       = "test-vhost2";
    uint32_t    connectionLimit1 = 100;
    uint32_t    connectionLimit2 = 200;

    // Adding limiter for vhostName1
    limiterManager.addConnectionRateLimiter(vhostName1, connectionLimit1);

    // Getting limiter for vhostName1
    std::shared_ptr<FixedWindowConnectionRateLimiter> limiter1 =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.getConnectionRateLimiter(vhostName1));
    ASSERT_TRUE(limiter1 != nullptr);
    EXPECT_EQ(limiter1->getConnectionLimit(), connectionLimit1);
    EXPECT_EQ(limiter1->getTimeWindowInSec(), 1);

    // Adding limiter for vhostName2
    std::shared_ptr<FixedWindowConnectionRateLimiter> limiter2 =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.addConnectionRateLimiter(vhostName2,
                                                    connectionLimit2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);

    // Modifying limiter for vhostName1
    uint32_t                                          newConnectionLimit = 300;
    std::shared_ptr<FixedWindowConnectionRateLimiter> newLimiter =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.addConnectionRateLimiter(vhostName1,
                                                    newConnectionLimit));
    ASSERT_TRUE(newLimiter != nullptr);
    EXPECT_EQ(newLimiter->getConnectionLimit(), newConnectionLimit);
    EXPECT_EQ(newLimiter->getTimeWindowInSec(), 1);

    // Getting limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
        limiterManager.getConnectionRateLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);

    // Removing limiter for vhostName1
    limiterManager.removeConnectionRateLimiter(vhostName1);
    std::shared_ptr<FixedWindowConnectionRateLimiter> removedLimiter =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.getConnectionRateLimiter(vhostName1));
    ASSERT_TRUE(removedLimiter == nullptr);

    // Getting limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
        limiterManager.getConnectionRateLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);
}

TEST(ConnectionLimiterManagerTest, AddGetRemoveAlarmOnlyConnectionRateLimiter)
{
    ConnectionLimiterManager limiterManager;

    std::string vhostName1       = "test-vhost1";
    std::string vhostName2       = "test-vhost2";
    uint32_t    connectionLimit1 = 100;
    uint32_t    connectionLimit2 = 200;

    // Adding alarm only limiter for vhostName1
    limiterManager.addAlarmOnlyConnectionRateLimiter(vhostName1,
                                                     connectionLimit1);

    // Getting alarm only limiter for vhostName1
    std::shared_ptr<FixedWindowConnectionRateLimiter> limiter1 =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.getAlarmOnlyConnectionRateLimiter(vhostName1));
    ASSERT_TRUE(limiter1 != nullptr);
    EXPECT_EQ(limiter1->getConnectionLimit(), connectionLimit1);
    EXPECT_EQ(limiter1->getTimeWindowInSec(), 1);

    // Adding alarm only limiter for vhostName2
    std::shared_ptr<FixedWindowConnectionRateLimiter> limiter2 =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.addAlarmOnlyConnectionRateLimiter(
                vhostName2, connectionLimit2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);

    // Modifying alarm only limiter for vhostName1
    uint32_t                                          newConnectionLimit = 300;
    std::shared_ptr<FixedWindowConnectionRateLimiter> newLimiter =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.addAlarmOnlyConnectionRateLimiter(
                vhostName1, newConnectionLimit));
    ASSERT_TRUE(newLimiter != nullptr);
    EXPECT_EQ(newLimiter->getConnectionLimit(), newConnectionLimit);
    EXPECT_EQ(newLimiter->getTimeWindowInSec(), 1);

    // Getting alarm only limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
        limiterManager.getAlarmOnlyConnectionRateLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);

    // Removing alarm only limiter for vhostName1
    limiterManager.removeAlarmOnlyConnectionRateLimiter(vhostName1);
    std::shared_ptr<FixedWindowConnectionRateLimiter> removedLimiter =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.getAlarmOnlyConnectionRateLimiter(vhostName1));
    ASSERT_TRUE(removedLimiter == nullptr);

    // Getting alarm only limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
        limiterManager.getAlarmOnlyConnectionRateLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getTimeWindowInSec(), 1);
}

TEST(ConnectionLimiterManagerTest, SetGetRemoveDefaultConnectionRateLimiter)
{
    ConnectionLimiterManager limiterManager;
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());

    uint32_t connectionLimit1 = 100;
    // Setting default limiter
    limiterManager.setDefaultConnectionRateLimit(connectionLimit1);

    // Getting default limiter
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit1);
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());

    uint32_t connectionLimit2 = 200;
    // Setting alarm only default limiter
    limiterManager.setAlarmOnlyDefaultConnectionRateLimit(connectionLimit2);

    // Getting alarm only default limiter
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultConnectionRateLimit(),
              connectionLimit2);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit1);

    // Removing default limiter
    limiterManager.removeDefaultConnectionRateLimit();

    // Getting default limiter
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultConnectionRateLimit(),
              connectionLimit2);

    // Removing alarm only default limiter
    limiterManager.removeAlarmOnlyDefaultConnectionRateLimit();

    // Getting default limiter
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
}

TEST(ConnectionLimiterManagerTest, AddGetRemoveTotalConnectionLimiter)
{
    ConnectionLimiterManager limiterManager;

    std::string vhostName1       = "test-vhost1";
    std::string vhostName2       = "test-vhost2";
    uint32_t    connectionLimit1 = 100;
    uint32_t    connectionLimit2 = 200;

    // Adding limiter for vhostName1
    limiterManager.addTotalConnectionLimiter(vhostName1, connectionLimit1);

    // Getting limiter for vhostName1
    std::shared_ptr<TotalConnectionLimiter> limiter1 =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.getTotalConnectionLimiter(vhostName1));
    ASSERT_TRUE(limiter1 != nullptr);
    EXPECT_EQ(limiter1->getTotalConnectionLimit(), connectionLimit1);
    EXPECT_EQ(limiter1->getConnectionCount(), 0);

    // Adding limiter for vhostName2
    std::shared_ptr<TotalConnectionLimiter> limiter2 =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.addTotalConnectionLimiter(vhostName2,
                                                     connectionLimit2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);

    // Modifying limiter for vhostName1
    uint32_t                                newConnectionLimit = 300;
    std::shared_ptr<TotalConnectionLimiter> newLimiter =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.addTotalConnectionLimiter(vhostName1,
                                                     newConnectionLimit));
    ASSERT_TRUE(newLimiter != nullptr);
    EXPECT_EQ(newLimiter->getTotalConnectionLimit(), newConnectionLimit);
    EXPECT_EQ(newLimiter->getConnectionCount(), 0);

    // Getting limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.getTotalConnectionLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);

    // Removing limiter for vhostName1
    limiterManager.removeTotalConnectionLimiter(vhostName1);
    std::shared_ptr<TotalConnectionLimiter> removedLimiter =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.getTotalConnectionLimiter(vhostName1));
    ASSERT_TRUE(removedLimiter == nullptr);

    // Getting limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.getTotalConnectionLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);
}

TEST(ConnectionLimiterManagerTest, AddGetRemoveAlarmOnlyTotalConnectionLimiter)
{
    ConnectionLimiterManager limiterManager;

    std::string vhostName1       = "test-vhost1";
    std::string vhostName2       = "test-vhost2";
    uint32_t    connectionLimit1 = 100;
    uint32_t    connectionLimit2 = 200;

    // Adding alarm only limiter for vhostName1
    limiterManager.addAlarmOnlyTotalConnectionLimiter(vhostName1,
                                                      connectionLimit1);

    // Getting alarm only limiter for vhostName1
    std::shared_ptr<TotalConnectionLimiter> limiter1 =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.getAlarmOnlyTotalConnectionLimiter(vhostName1));
    ASSERT_TRUE(limiter1 != nullptr);
    EXPECT_EQ(limiter1->getTotalConnectionLimit(), connectionLimit1);
    EXPECT_EQ(limiter1->getConnectionCount(), 0);

    // Adding alarm only limiter for vhostName2
    std::shared_ptr<TotalConnectionLimiter> limiter2 =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.addAlarmOnlyTotalConnectionLimiter(
                vhostName2, connectionLimit2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);

    // Modifying alarm only limiter for vhostName1
    uint32_t                                newConnectionLimit = 300;
    std::shared_ptr<TotalConnectionLimiter> newLimiter =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.addAlarmOnlyTotalConnectionLimiter(
                vhostName1, newConnectionLimit));
    ASSERT_TRUE(newLimiter != nullptr);
    EXPECT_EQ(newLimiter->getTotalConnectionLimit(), newConnectionLimit);
    EXPECT_EQ(newLimiter->getConnectionCount(), 0);

    // Getting alarm only limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.getAlarmOnlyTotalConnectionLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);

    // Removing alarm only limiter for vhostName1
    limiterManager.removeAlarmOnlyTotalConnectionLimiter(vhostName1);
    std::shared_ptr<TotalConnectionLimiter> removedLimiter =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.getAlarmOnlyTotalConnectionLimiter(vhostName1));
    ASSERT_TRUE(removedLimiter == nullptr);

    // Getting alarm only limiter for vhostName2
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.getAlarmOnlyTotalConnectionLimiter(vhostName2));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), connectionLimit2);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);
}

TEST(ConnectionLimiterManagerTest, SetGetRemoveDefaultTotalConnectionLimiter)
{
    ConnectionLimiterManager limiterManager;
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());

    uint32_t connectionLimit1 = 100;
    // Setting default limiter
    limiterManager.setDefaultTotalConnectionLimit(connectionLimit1);

    // Getting default limiter
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit1);
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());

    uint32_t connectionLimit2 = 200;
    // Setting alarm only default limiter
    limiterManager.setAlarmOnlyDefaultTotalConnectionLimit(connectionLimit2);

    // Getting alarm only default limiter
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultTotalConnectionLimit(),
              connectionLimit2);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit1);

    // Removing default limiter
    limiterManager.removeDefaultTotalConnectionLimit();

    // Getting default limiter
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultTotalConnectionLimit(),
              connectionLimit2);

    // Removing alarm only default limiter
    limiterManager.removeAlarmOnlyDefaultTotalConnectionLimit();

    // Getting default limiter
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
}

TEST(ConnectionLimiterManagerTest, AllowNewConnectionForVhostWithoutAnyLimit)
{
    ConnectionLimiterManager limiterManager;
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost("test-vhost"));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost("test-vhost"));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithSpecificRateLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.addConnectionRateLimiter(vhostName, connectionLimit);
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithAlarmOnlySpecificRateLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.addAlarmOnlyConnectionRateLimiter(vhostName,
                                                     connectionLimit);
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithDefaultRateLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setDefaultConnectionRateLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithAlarmOnlyDefaultRateLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setAlarmOnlyDefaultConnectionRateLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithSpecificAndDefaultRateLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setDefaultConnectionRateLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t newConnectionLimit = 2;
    limiterManager.addConnectionRateLimiter(vhostName, newConnectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    limiterManager.removeConnectionRateLimiter(vhostName);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithSpecificTotalConnectionLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.addTotalConnectionLimiter(vhostName, connectionLimit);
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithAlarmOnlySpecificTotalConnectionLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.addAlarmOnlyTotalConnectionLimiter(vhostName,
                                                      connectionLimit);
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithDefaultTotalConnectionLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setDefaultTotalConnectionLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithAlarmOnlyDefaultTotalConnectionLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setAlarmOnlyDefaultTotalConnectionLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getAlarmOnlyDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getAlarmOnlyDefaultTotalConnectionLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithSpecificAndDefaultTotalConnectionLimit)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 1;
    limiterManager.setDefaultTotalConnectionLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t newConnectionLimit = 2;
    limiterManager.addTotalConnectionLimiter(vhostName, newConnectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    limiterManager.removeTotalConnectionLimiter(vhostName);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              connectionLimit);
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(ConnectionLimiterManagerTest,
     AllowNewConnectionForVhostWithDefaultRateLimiterAndTotalConnectionLimiter)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";
    EXPECT_FALSE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 5;
    limiterManager.setDefaultConnectionRateLimit(connectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultConnectionRateLimit());
    EXPECT_EQ(*limiterManager.getDefaultConnectionRateLimit(),
              connectionLimit);
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());

    uint32_t totalConnectionLimit = 1;
    limiterManager.setDefaultTotalConnectionLimit(totalConnectionLimit);
    ASSERT_TRUE(limiterManager.getDefaultTotalConnectionLimit());
    EXPECT_EQ(*limiterManager.getDefaultTotalConnectionLimit(),
              totalConnectionLimit);

    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));

    limiterManager.removeDefaultTotalConnectionLimit();
    EXPECT_FALSE(limiterManager.getDefaultTotalConnectionLimit());

    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}

TEST(
    ConnectionLimiterManagerTest,
    AllowNewConnectionForVhostWithSpecificRateLimiterAndTotalConnectionLimiter)
{
    ConnectionLimiterManager limiterManager;
    std::string              vhostName = "test-vhost";

    std::shared_ptr<FixedWindowConnectionRateLimiter> limiter1 =
        std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
            limiterManager.getConnectionRateLimiter(vhostName));
    EXPECT_FALSE(limiter1 != nullptr);
    std::shared_ptr<TotalConnectionLimiter> limiter2 =
        std::dynamic_pointer_cast<TotalConnectionLimiter>(
            limiterManager.getTotalConnectionLimiter(vhostName));
    EXPECT_FALSE(limiter2 != nullptr);

    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));

    uint32_t connectionLimit = 5;
    limiter1 = std::dynamic_pointer_cast<FixedWindowConnectionRateLimiter>(
        limiterManager.addConnectionRateLimiter(vhostName, connectionLimit));
    ASSERT_TRUE(limiter1 != nullptr);
    EXPECT_EQ(limiter1->getConnectionLimit(), connectionLimit);
    EXPECT_EQ(limiter1->getTimeWindowInSec(), 1);

    uint32_t totalConnectionLimit = 1;
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.addTotalConnectionLimiter(vhostName,
                                                 totalConnectionLimit));
    ASSERT_TRUE(limiter2 != nullptr);
    EXPECT_EQ(limiter2->getTotalConnectionLimit(), totalConnectionLimit);
    EXPECT_EQ(limiter2->getConnectionCount(), 0);

    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_FALSE(limiterManager.allowNewConnectionForVhost(vhostName));

    limiterManager.removeTotalConnectionLimiter(vhostName);
    limiter2 = std::dynamic_pointer_cast<TotalConnectionLimiter>(
        limiterManager.getTotalConnectionLimiter(vhostName));
    EXPECT_FALSE(limiter2 != nullptr);

    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
    EXPECT_TRUE(limiterManager.allowNewConnectionForVhost(vhostName));
}
