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
#include <amqpprox_backend.h>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::Backend;

TEST(Backend, Breathing)
{
    Backend backend;

    EXPECT_TRUE(true);
}

TEST(Backend, RetrieveAllValues)
{
    Backend backend("name", "datacenter", "host", "backend-ip", 100);

    EXPECT_EQ("name", backend.name());
    EXPECT_EQ("datacenter", backend.datacenterTag());
    EXPECT_EQ("host", backend.host());
    EXPECT_EQ("backend-ip", backend.ip());
    EXPECT_EQ(100, backend.port());
    EXPECT_FALSE(backend.proxyProtocolEnabled());
    EXPECT_FALSE(backend.tlsEnabled());
}

TEST(Backend, RetrieveExtendedValues_Proxy)
{
    Backend backend(
        "name", "datacenter", "host", "backend-ip", 100, true, false);

    EXPECT_EQ("name", backend.name());
    EXPECT_EQ("datacenter", backend.datacenterTag());
    EXPECT_EQ("host", backend.host());
    EXPECT_EQ("backend-ip", backend.ip());
    EXPECT_EQ(100, backend.port());
    EXPECT_TRUE(backend.proxyProtocolEnabled());
    EXPECT_FALSE(backend.tlsEnabled());
}

TEST(Backend, RetrieveExtendedValues_Tls)
{
    Backend backend(
        "name", "datacenter", "host", "backend-ip", 100, false, true);

    EXPECT_EQ("name", backend.name());
    EXPECT_EQ("datacenter", backend.datacenterTag());
    EXPECT_EQ("host", backend.host());
    EXPECT_EQ("backend-ip", backend.ip());
    EXPECT_EQ(100, backend.port());
    EXPECT_FALSE(backend.proxyProtocolEnabled());
    EXPECT_TRUE(backend.tlsEnabled());
}
