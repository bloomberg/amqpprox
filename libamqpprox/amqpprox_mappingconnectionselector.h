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
#ifndef BLOOMBERG_AMQPPROX_MAPPINGCONNECTIONSELECTOR
#define BLOOMBERG_AMQPPROX_MAPPINGCONNECTIONSELECTOR

#include <amqpprox_connectionselector.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

class FarmStore;
class BackendStore;
class ResourceMapper;

/**
 * \brief Determines where to make the egress connection(proxy to broker),
 * implements the ConnectionSelector interface
 */
class MappingConnectionSelector : public ConnectionSelector {
    FarmStore                               *d_farmStore_p;
    BackendStore                            *d_backendStore_p;
    ResourceMapper                          *d_resourceMapper_p;
    std::string                              d_defaultFarmName;
    std::unordered_map<std::string, uint8_t> d_indexes;
    mutable std::mutex                       d_mutex;

  public:
    // CREATORS
    /**
     * \brief Construct a MappingConnectionSelector
     * \param farmStore
     * \param backendStore
     * \param resourceMapper
     */
    MappingConnectionSelector(FarmStore      *farmStore,
                              BackendStore   *backendStore,
                              ResourceMapper *resourceMapper);

    virtual ~MappingConnectionSelector();

    // MANIPULATORS
    /**
     * \brief Acquire a connection from the specified session `state` and set
     * `connectionOut` to be a `ConnectionManager` instance tracking connection
     * attempt.
     * \return zero on success, or a non-zero value otherwise
     */
    virtual int
    acquireConnection(std::shared_ptr<ConnectionManager> *connectionOut,
                      const SessionState                 &state) override;

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
