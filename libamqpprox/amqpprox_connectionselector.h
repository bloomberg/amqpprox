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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONSELECTOR
#define BLOOMBERG_AMQPPROX_CONNECTIONSELECTOR

#include <amqpprox_connectionselectorinterface.h>

#include <amqpprox_connectionlimitermanager.h>
#include <amqpprox_sessionstate.h>
#include <memory>
#include <mutex>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class FarmStore;
class BackendStore;
class ResourceMapper;
class ConnectionLimiterManager;

/**
 * \brief Determines whether the incoming connection from client should be
 * limited and then where to make the egress connection(proxy to broker),
 * implements the ConnectionSelectorInterface
 */
class ConnectionSelector : public ConnectionSelectorInterface {
    FarmStore                *d_farmStore_p;
    BackendStore             *d_backendStore_p;
    ResourceMapper           *d_resourceMapper_p;
    std::string               d_defaultFarmName;
    ConnectionLimiterManager *d_connectionLimiterManager_p;
    mutable std::mutex        d_mutex;

  public:
    // CREATORS
    /**
     * \brief Construct a ConnectionSelector
     * \param farmStore
     * \param backendStore
     * \param resourceMapper
     */
    ConnectionSelector(FarmStore                *farmStore,
                       BackendStore             *backendStore,
                       ResourceMapper           *resourceMapper,
                       ConnectionLimiterManager *connectionLimiterManager);

    virtual ~ConnectionSelector();

    // MANIPULATORS
    /**
     * \brief Acquire a connection from the specified session `state` and set
     * `connectionOut` to be a `ConnectionManager` instance tracking connection
     * attempt.
     * \return connection status to represent whether the connection should go
     * through
     */
    virtual SessionState::ConnectionStatus
    acquireConnection(std::shared_ptr<ConnectionManager> *connectionOut,
                      const SessionState                 &state) override;

    /**
     * \brief Notify connection disconnect event
     */
    virtual void
    notifyConnectionDisconnect(const std::string &vhostName) override;

    /**
     * \brief Set the default farm if a mapping is not found
     */
    void setDefaultFarm(const std::string &farmName);

    /**
     * \brief Unset any default farm if a mapping is not found
     */
    void unsetDefaultFarm();
};

}
}

#endif
