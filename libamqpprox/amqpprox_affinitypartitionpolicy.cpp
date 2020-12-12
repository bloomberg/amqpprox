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
#include <amqpprox_affinitypartitionpolicy.h>

#include <amqpprox_backendset.h>
#include <amqpprox_datacenter.h>

#include <memory>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

namespace {

const std::string POLICY_NAME("datacenter-affinity");

}

AffinityPartitionPolicy::AffinityPartitionPolicy(Datacenter *datacenter)
: d_datacenter_p(datacenter)
{
}

std::shared_ptr<BackendSet> AffinityPartitionPolicy::partition(
    const std::shared_ptr<BackendSet> &backendSet)
{
    std::vector<BackendSet::Partition> partitions;

    // Split all existing partitions into two new partitions. The first will
    // contain items that match the affinity value of this policy, and the
    // second will contain all other elements.
    for (const auto &partition : backendSet->partitions()) {
        BackendSet::Partition withAffinity;
        BackendSet::Partition noAffinity;

        for (const auto &backend : partition) {
            if (backend->datacenterTag() == d_datacenter_p->get()) {
                withAffinity.push_back(backend);
            }
            else {
                noAffinity.push_back(backend);
            }
        }

        // Copy the partitions to the output `BackendSet` with the affinity
        // partition first, excluding any empty partitions.
        if (withAffinity.size() > 0) {
            partitions.push_back(std::move(withAffinity));
        }
        if (noAffinity.size() > 0) {
            partitions.push_back(std::move(noAffinity));
        }
    }

    return std::make_shared<BackendSet>(std::move(partitions));
}

inline const std::string &AffinityPartitionPolicy::policyName() const
{
    return POLICY_NAME;
}

}
}
