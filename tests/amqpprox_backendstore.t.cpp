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

#include <amqpprox_backendstore.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(BackendStore, Breathing)
{
    BackendStore store;
}

TEST(BackendStore, BasicInsert)
{
    BackendStore store;
    Backend      backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    Backend backend2(
        "backend2", "dc2", "backend2.bloomberg.com", "127.0.0.2", 5672, true);

    EXPECT_EQ(store.insert(backend1), 0);
    EXPECT_EQ(store.insert(backend2), 0);
}

TEST(BackendStore, LookupEmpty)
{
    BackendStore store;
    EXPECT_EQ(store.lookup("backend3"), nullptr);
}

TEST(BackendStore, Lookup_Items_By_Name)
{
    BackendStore store;
    Backend      backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    Backend backend2(
        "backend2", "dc2", "backend2.bloomberg.com", "127.0.0.2", 5672, true);

    EXPECT_EQ(store.insert(backend1), 0);
    EXPECT_EQ(store.insert(backend2), 0);

    // Non existing still fails
    EXPECT_EQ(store.lookup("backend3"), nullptr);

    auto b1 = store.lookup("backend1");
    auto b2 = store.lookup("backend2");

    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);
    EXPECT_EQ(*b1, backend1);
    EXPECT_EQ(*b2, backend2);
}

TEST(BackendStore, CollidingItems)
{
    BackendStore store;
    Backend      backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    Backend backend2(
        "backend1", "dc2", "backend2.bloomberg.com", "127.0.0.2", 5672, true);

    EXPECT_EQ(store.insert(backend1), 0);
    EXPECT_NE(store.insert(backend2), 0);
}

TEST(BackendStore, Print_Breathing_Test)
{
    BackendStore store;
    Backend      backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    Backend backend2(
        "backend2", "dc2", "backend2.bloomberg.com", "127.0.0.2", 5672, true);

    EXPECT_EQ(store.insert(backend1), 0);
    EXPECT_EQ(store.insert(backend2), 0);

    std::ostringstream oss;
    store.print(oss);

    // NB: we explicitly don't prescribe any format of the output, we just
    // expect some
    EXPECT_GT(oss.str().length(), 0);
}

TEST(BackendStore, Removals)
{
    BackendStore store;
    Backend      backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    Backend backend2(
        "backend2", "dc2", "backend2.bloomberg.com", "127.0.0.2", 5672, true);

    EXPECT_EQ(store.insert(backend1), 0);
    EXPECT_EQ(store.insert(backend2), 0);

    // Check both backends are in place
    EXPECT_EQ(*store.lookup("backend1"), backend1);
    EXPECT_EQ(*store.lookup("backend2"), backend2);

    // Try removing a non-existing backend
    EXPECT_EQ(store.remove("non-existing"), 1);

    // Check both backends are in place
    EXPECT_EQ(*store.lookup("backend1"), backend1);
    EXPECT_EQ(*store.lookup("backend2"), backend2);

    // Remove backend1
    EXPECT_EQ(store.remove("backend1"), 0);

    // Check backend1 is missing and backend2 is still in place
    EXPECT_EQ(store.lookup("backend1"), nullptr);
    EXPECT_EQ(*store.lookup("backend2"), backend2);

    // Remove backend2
    EXPECT_EQ(store.remove("backend2"), 0);

    // Check both backends are missing
    EXPECT_EQ(store.lookup("backend1"), nullptr);
    EXPECT_EQ(store.lookup("backend2"), nullptr);
}
