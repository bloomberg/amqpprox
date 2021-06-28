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
#ifndef BLOOMBERG_AMQPPROX_AFFINITYPARTITIONPOLICY
#define BLOOMBERG_AMQPPROX_AFFINITYPARTITIONPOLICY

#include <amqpprox_partitionpolicy.h>

#include <memory>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Datacenter;

/**
 * \brief Represents affinity partition policy
 */
class AffinityPartitionPolicy : public PartitionPolicy {
  private:
    // DATA
    Datacenter *d_datacenter_p;

  public:
    // CREATORS
    explicit AffinityPartitionPolicy(Datacenter *d_datacenter);

    virtual ~AffinityPartitionPolicy() override = default;

    // MANIPULATORS
    /**
     * \brief Partition the specified `backendSet` according to datacenter
     * affinity
     * \return Shared pointer to the new `BackendSet` that was
     * partitioned according to this strategy
     *
     * Every partition currently in the `BackendSet` will be split into two new
     * partitions. The first partition will contain elements that match the
     * affinity of this policy and the second will contain all other elements.
     * The resulting partitions will be merged, in order, to form a new
     * `BackendSet` excluding any empty partitions.
     */
    virtual std::shared_ptr<BackendSet>
    partition(const std::shared_ptr<BackendSet> &backendSet) override;

    // ACCESSORS
    /**
     * \return Name of this `PartitionPolicy`. This name is used to attach this
     * policy to a given `Farm`.
     */
    virtual const std::string &policyName() const override;
};

}
}

#endif
