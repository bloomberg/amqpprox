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
#ifndef BLOOMBERG_AMQPPROX_FARM
#define BLOOMBERG_AMQPPROX_FARM

#include <amqpprox_backendselector.h>
#include <amqpprox_backendset.h>
#include <amqpprox_backendstore.h>
#include <amqpprox_partitionpolicy.h>

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Encapsulates a farm of 'backend' nodes
 */
class Farm {
    std::string                     d_name;
    std::unordered_set<std::string> d_backendMembers;

    BackendStore *                 d_backendStore_p;     // HELD NOT OWNED
    BackendSelector *              d_backendSelector_p;  // HELD NOT OWNED
    std::vector<PartitionPolicy *> d_partitionPolicies;
    std::shared_ptr<BackendSet>    d_backendSet;

    mutable std::mutex d_mutex;

    // PRIVATE MANIPULATORS

    /**
     * \brief Replace the `BackendSet` instance for this farm with the
     * specified `backendSet`.
     * \param backendSet shared pointer to a `BackendSet`
     */
    void setBackendSet(std::shared_ptr<BackendSet> backendSet);

    /**
     * \brief Perform repartitioning of the `BackendSet` for this farm. The
     * behaviour of this function is undefined unless it is called within a
     * lock on `d_mutex`.
     * \param guard for the mutex to prove that we're safe during the critical
     * section
     */
    void doRepartitionWhileLocked(std::lock_guard<std::mutex> &guard);

  public:
    // CREATORS
    /**
     * \brief construct a Farm object
     * \param name of the farm
     * \param members vector the members addresses in string form
     * \param backendStore pointer the the `BackendStore` of members
     * \param backendSelector pointer to the `BackendSelector` logic used to
     * partition members
     */
    Farm(const std::string &             name,
         const std::vector<std::string> &members,
         BackendStore *                  backendStore,
         BackendSelector *               backendSelector);

    // MANIPULATORS
    /**
     * \brief add a backend node to the farm
     * \param backend string containing the backend's address
     */
    void addMember(const std::string &backend);

    /**
     * \brief remove a backend node from the farm
     * \param backend string containing the backend's address
     */
    void removeMember(const std::string &backend);

    /**
     * \brief Set the selector used for this `Farm` to the specified
     * `selector`.
     * \param selector the `BackendSelector` to be used.
     */
    void setBackendSelector(BackendSelector *selector);

    /**
     * \brief Add the specified `partitionPolicy` to this `Farm`.
     * \param partitionPolicy pointer to the `PartitionPolicy` logic to
     * partition the members
     */
    void addPartitionPolicy(PartitionPolicy *partitionPolicy);

    /**
     * \brief Recalculate the `BackendSet` for this `Farm` using the ordered
     * vector of `PartitionPolicy` instances attached to this `Farm`.
     */
    void repartition();

    // ACCESSORS
    /**
     * \returns name of the farm
     */
    const std::string &name() const;

    /**
     * \param members will be populated with the addresses of
     * the farm members
     */
    void members(std::vector<std::string> *members) const;

    /**
     * \returns shared pointer to the `BackendSet`
     */
    std::shared_ptr<BackendSet> backendSet() const;

    /**
     * \returns pointer to the `BackendSelector` in play for the farm
     */
    BackendSelector *backendSelector() const;
};

std::ostream &operator<<(std::ostream &os, const Farm &farm);

}
}

#endif
