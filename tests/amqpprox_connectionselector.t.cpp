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

#include <amqpprox_backendstore.h>
#include <amqpprox_connectionmanager.h>
#include <amqpprox_connectionselector.h>
#include <amqpprox_farmstore.h>
#include <amqpprox_resourcemapper.h>
#include <amqpprox_robinbackendselector.h>
#include <amqpprox_sessionstate.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(ConnectionSelector, Breathing)
{
    FarmStore          farmStore;
    BackendStore       backendStore;
    ResourceMapper     resourceMapper;
    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);
}

TEST(ConnectionSelector, Find_Nothing)
{
    FarmStore          farmStore;
    BackendStore       backendStore;
    ResourceMapper     resourceMapper;
    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);
    SessionState                       state;
    std::shared_ptr<ConnectionManager> out;
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::NO_FARM);
}

TEST(ConnectionSelector, Find_Nothing_Defaulted)
{
    FarmStore          farmStore;
    BackendStore       backendStore;
    ResourceMapper     resourceMapper;
    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);

    connectionSelector.setDefaultFarm("DEFAULT");
    SessionState                       state;
    std::shared_ptr<ConnectionManager> out;
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::ERROR_FARM);

    // Check after unsetting the default we go back to the previous rc
    connectionSelector.unsetDefaultFarm();
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::NO_FARM);
}

TEST(ConnectionSelector, Successful_Farm_Find)
{
    FarmStore                farmStore;
    BackendStore             backendStore;
    ResourceMapper           resourceMapper;
    RobinBackendSelector     backendSelector;
    std::vector<std::string> members;
    farmStore.addFarm(std::make_unique<Farm>(
        "DEFAULT", members, &backendStore, &backendSelector));
    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);

    connectionSelector.setDefaultFarm("DEFAULT");
    SessionState                       state;
    std::shared_ptr<ConnectionManager> out;
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::SUCCESS);
}

TEST(ConnectionSelector, Find_Nothing_Backend)
{
    FarmStore      farmStore;
    BackendStore   backendStore;
    ResourceMapper resourceMapper;

    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);
    SessionState state;

    state.setVirtualHost("/");
    resourceMapper.mapVhostToBackend("/", "non-existing");
    std::shared_ptr<ConnectionManager> out;
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::NO_BACKEND);
}

TEST(ConnectionSelector, Successful_Backend_Find)
{
    FarmStore            farmStore;
    BackendStore         backendStore;
    ResourceMapper       resourceMapper;
    RobinBackendSelector backendSelector;

    Backend backend1(
        "backend1", "dc1", "backend1.bloomberg.com", "127.0.0.1", 5672, true);
    backendStore.insert(backend1);

    ConnectionSelector connectionSelector(
        &farmStore, &backendStore, &resourceMapper);
    SessionState state;

    state.setVirtualHost("/");
    resourceMapper.mapVhostToBackend("/", "backend1");
    std::shared_ptr<ConnectionManager> out;
    EXPECT_EQ(connectionSelector.acquireConnection(&out, state),
              SessionState::ConnectionStatus::SUCCESS);
}