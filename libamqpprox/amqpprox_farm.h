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

class Farm {
    std::string                     d_name;
    std::unordered_set<std::string> d_backendMembers;

    BackendStore *                 d_backendStore_p;     // HELD NOT OWNED
    BackendSelector *              d_backendSelector_p;  // HELD NOT OWNED
    std::vector<PartitionPolicy *> d_partitionPolicies;
    std::shared_ptr<BackendSet>    d_backendSet;

    std::string        d_dnsName;
    int                d_dnsPort;
    mutable std::mutex d_mutex;

    // PRIVATE MANIPULATORS
    void setBackendSet(std::shared_ptr<BackendSet> backendSet);
    ///< Replace the `BackendSet` instance for this farm with the specified
    ///< `backendSet`.

    void doRepartitionWhileLocked(std::lock_guard<std::mutex> &guard);
    ///< Perform repartitioning of the `BackendSet` for this farm. The
    ///< behaviour of this function is undefined unless it is called within
    ///< a lock on `d_mutex`.

  public:
    // CREATORS
    Farm(const std::string &             name,
         const std::vector<std::string> &members,
         BackendStore *                  backendStore,
         BackendSelector *               backendSelector);
    Farm(const std::string &name, const std::string &dnsName, int dnsPort);

    // MANIPULATORS
    void addMember(const std::string &backend);
    void removeMember(const std::string &backend);
    void setDns(const std::string &name, int port);

    void setBackendSelector(BackendSelector *selector);
    ///< Set the selector used for this `Farm` to the specified `selector`.

    void addPartitionPolicy(PartitionPolicy *partitionPolicy);
    ///< Add the specified `partitionPolicy` to this `Farm`.

    void repartition();
    ///< Recalculate the `BackendSet` for this `Farm` using the ordered
    ///< vector of `PartitionPolicy` instances attached to this `Farm`.

    // ACCESSORS
    const std::string &name() const;
    const std::string &dnsName() const;
    int                dnsPort() const;
    void               members(std::vector<std::string> *members) const;
    std::shared_ptr<BackendSet> backendSet() const;
    BackendSelector *           backendSelector() const;
};

std::ostream &operator<<(std::ostream &os, const Farm &farm);

}
}

#endif
