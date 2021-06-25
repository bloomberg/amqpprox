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
#ifndef BLOOMBERG_AMQPPROX_PARTITIONPOLICYSTORE
#define BLOOMBERG_AMQPPROX_PARTITIONPOLICYSTORE

#include <amqpprox_partitionpolicy.h>

#include <map>
#include <memory>
#include <string>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Stores different partition policies for BackendSelector
 */
class PartitionPolicyStore {
  private:
    // DATA
    std::map<std::string, std::unique_ptr<PartitionPolicy>> d_policies;

  public:
    // MANIPULATORS
    /**
     * \brief Add the specified `PartitionPolicy` to the map of named policies
     * installed in this store.
     */
    void addPolicy(std::unique_ptr<PartitionPolicy> policy);

    // ACCESSORS
    /**
     * \return a pointer to the `PartitionPolicy` instance with the specified
     * `name`.
     */
    PartitionPolicy *getPolicy(const std::string &name) const;
};

}
}

#endif
