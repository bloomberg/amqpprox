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
#include <amqpprox_farmstore.h>

#include <gtest/gtest.h>

#include <iostream>

using Bloomberg::amqpprox::Farm;
using Bloomberg::amqpprox::FarmStore;

TEST(FarmStore, Breathing)
{
    FarmStore farmStore;

    EXPECT_TRUE(true);
}

TEST(FarmStore, AddingFarm)
{
    FarmStore farmStore;

    std::unique_ptr<Farm> farm(new Farm("testfarm", "testdns", 100));
    farmStore.addFarm(std::move(farm));

    std::ostringstream oss;
    farmStore.print(oss);
    EXPECT_EQ(oss.str(), "testfarm [testdns:100]: \n");
}

TEST(FarmStore, RemovingFarm)
{
    FarmStore farmStore;

    std::unique_ptr<Farm> farm(new Farm("testfarm", "testdns", 100));
    farmStore.addFarm(std::move(farm));

    farmStore.removeFarmByName("testfarm");

    std::ostringstream oss;
    farmStore.print(oss);
    EXPECT_EQ(oss.str(), "");
}

TEST(FarmStore, GettingExistingFarm)
{
    FarmStore farmStore;

    std::unique_ptr<Farm> farm(new Farm("testfarm", "testdns", 100));
    farmStore.addFarm(std::move(farm));

    Farm &retrievedFarm = farmStore.getFarmByName("testfarm");

    EXPECT_EQ("testfarm", retrievedFarm.name());
    EXPECT_EQ("testdns", retrievedFarm.dnsName());
}

TEST(FarmStore, GettingMissingFarm)
{
    FarmStore farmStore;

    EXPECT_THROW({ farmStore.getFarmByName("nosuchfarm"); },
                 std::runtime_error);
}

TEST(FarmStore, AddTwice)
{
    FarmStore farmStore;

    std::unique_ptr<Farm> farm(new Farm("testfarm", "testdns", 100));
    farmStore.addFarm(std::move(farm));

    std::ostringstream oss;
    farmStore.print(oss);
    EXPECT_EQ(oss.str(), "testfarm [testdns:100]: \n");

    std::unique_ptr<Farm> farm2(new Farm("testfarm", "testdns2", 101));
    farmStore.addFarm(std::move(farm2));

    std::ostringstream oss2;
    farmStore.print(oss2);
    EXPECT_EQ(oss2.str(), "testfarm [testdns2:101]: \n");
}
