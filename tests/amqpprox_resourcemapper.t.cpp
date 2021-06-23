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
#include <amqpprox_resourcemapper.h>

#include <amqpprox_sessionstate.h>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::ResourceMapper;
using Bloomberg::amqpprox::SessionState;

TEST(ResourceMapper, Breathing)
{
    ResourceMapper mapper;
}

TEST(ResourceMapper, BasicMap)
{
    ResourceMapper mapper;

    SessionState state(nullptr);
    state.setVirtualHost("/");
    std::string resourceName;
    bool        isFarm = false;

    EXPECT_FALSE(mapper.getResourceMap(&isFarm, &resourceName, state));

    mapper.mapVhostToFarm("/alaric", "dedicated1");
    mapper.mapVhostToFarm("/", "shared1");

    EXPECT_TRUE(mapper.getResourceMap(&isFarm, &resourceName, state));
    EXPECT_TRUE(isFarm);
    EXPECT_EQ(resourceName, std::string("shared1"));

    mapper.mapVhostToBackend("/", "shared1-ny1");
    EXPECT_TRUE(mapper.getResourceMap(&isFarm, &resourceName, state));
    EXPECT_FALSE(isFarm);
    EXPECT_EQ(resourceName, std::string("shared1-ny1"));

    mapper.unmapVhost("/");
    EXPECT_FALSE(mapper.getResourceMap(&isFarm, &resourceName, state));
}

TEST(ResourceMapper, Print)
{
    ResourceMapper mapper;
    mapper.mapVhostToFarm("/alaric", "dedicated1");
    mapper.mapVhostToFarm("/", "shared1");
    mapper.mapVhostToBackend("/vas", "shared1-ny1");

    std::ostringstream oss;
    mapper.print(oss);
    EXPECT_EQ(oss.str(),
              std::string("\"/\" => Farm:shared1\n"
                          "\"/alaric\" => Farm:dedicated1\n"
                          "\"/vas\" => Backend:shared1-ny1\n"));
}
