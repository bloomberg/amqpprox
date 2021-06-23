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
#ifndef BLOOMBERG_AMQPPROX_PARTITIONPOLICY
#define BLOOMBERG_AMQPPROX_PARTITIONPOLICY

#include <memory>

namespace Bloomberg {
namespace amqpprox {

class BackendSet;

/**
 * \brief Interface to implement different partition policies for
 * BackendSelector
 */
class PartitionPolicy {
  public:
    // CREATORS
    virtual ~PartitionPolicy() = default;

    // MANIPULATORS
    /**
     * \brief Modify the partitioning of the specified `backendSet` and return
     * a shared_ptr to a new `BackendSet` instance with the modifications
     * applied. When making a connection, a `Server` will attempt all
     * connections in the first partition (in an implementation defined order),
     * then the second, and so on until all connections have been exhausted.
     */
    virtual std::shared_ptr<BackendSet>
    partition(const std::shared_ptr<BackendSet> &backendSet) = 0;

    // ACCESSORS
    /**
     * \return the name of this `PartitionPolicy`. This name is used to attach
     * this policy to a given `Farm`.
     */
    virtual const std::string &policyName() const = 0;
};

}
}

#endif
