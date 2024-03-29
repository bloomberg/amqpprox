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
#include <amqpprox_connectionselector.h>

#include <amqpprox_backend.h>
#include <amqpprox_backendset.h>
#include <amqpprox_backendstore.h>
#include <amqpprox_connectionlimitermanager.h>
#include <amqpprox_connectionmanager.h>
#include <amqpprox_farmstore.h>
#include <amqpprox_logging.h>
#include <amqpprox_resourcemapper.h>
#include <amqpprox_sessionstate.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

ConnectionSelector::ConnectionSelector(
    FarmStore                *farmStore,
    BackendStore             *backendStore,
    ResourceMapper           *resourceMapper,
    ConnectionLimiterManager *connectionLimiterManager)
: d_farmStore_p(farmStore)
, d_backendStore_p(backendStore)
, d_resourceMapper_p(resourceMapper)
, d_defaultFarmName("")
, d_connectionLimiterManager_p(connectionLimiterManager)
, d_mutex()
{
}

ConnectionSelector::~ConnectionSelector()
{
}

SessionState::ConnectionStatus ConnectionSelector::acquireConnection(
    std::shared_ptr<ConnectionManager> *connectionOut,
    const SessionState                 &sessionState)
{
    std::shared_ptr<ConnectionManager> connectionManager;

    bool        isFarm = false;
    std::string resourceName;

    if (!(d_connectionLimiterManager_p->allowNewConnectionForVhost(
            sessionState.getVirtualHost()))) {
        // The current request will be limited based on different connection
        // limiters
        LOG_DEBUG << "The connection request for "
                  << sessionState.getVirtualHost() << " is limited by proxy.";

        return SessionState::ConnectionStatus::LIMIT;
    }

    if (!d_resourceMapper_p->getResourceMap(
            &isFarm, &resourceName, sessionState)) {
        std::lock_guard<std::mutex> lg(d_mutex);
        if (d_defaultFarmName.empty()) {
            LOG_INFO << "No farm available for: " << sessionState;
            return SessionState::ConnectionStatus::NO_FARM;
        }
        else {
            isFarm       = true;
            resourceName = d_defaultFarmName;
        }
    }

    if (isFarm) {
        // Return the BackendSet and BackendSelector generated by the Farm
        try {
            const auto &farm = d_farmStore_p->getFarmByName(resourceName);

            connectionManager.reset(new ConnectionManager(
                farm.backendSet(), farm.backendSelector()));
        }
        catch (std::runtime_error &e) {
            LOG_WARN << "Unable to acquire backend from Farm: " << resourceName
                     << " for: " << sessionState;
            return SessionState::ConnectionStatus::ERROR_FARM;
        }

        LOG_INFO << "Selected farm: " << resourceName << " For "
                 << sessionState;
    }
    else {
        // Construct a BackendSet directly and pass a nullptr BackendSelector
        auto backend = d_backendStore_p->lookup(resourceName);
        if (!backend) {
            return SessionState::ConnectionStatus::NO_BACKEND;
        }

        std::vector<BackendSet::Partition> partitions = {{backend}};

        connectionManager.reset(new ConnectionManager(
            std::make_shared<BackendSet>(std::move(partitions)), nullptr));

        LOG_INFO << "Selected directly: " << *backend << " For "
                 << sessionState;
    }

    connectionOut->swap(connectionManager);

    return SessionState::ConnectionStatus::SUCCESS;
}

void ConnectionSelector::setDefaultFarm(const std::string &farmName)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_defaultFarmName = farmName;
}

void ConnectionSelector::unsetDefaultFarm()
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_defaultFarmName = "";
}

}
}
