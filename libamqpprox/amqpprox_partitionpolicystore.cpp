/*
** Copyright 2021 Bloomberg Finance L.P.
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
#include <amqpprox_partitionpolicystore.h>

#include <amqpprox_partitionpolicy.h>

#include <string>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

// MANIPULATORS
void PartitionPolicyStore::addPolicy(std::unique_ptr<PartitionPolicy> policy)
{
    d_policies[policy->policyName()] = std::move(policy);
}

// ACCESSORS
PartitionPolicy *PartitionPolicyStore::getPolicy(const std::string &name) const
{
    const auto &it = d_policies.find(name);
    if (it == d_policies.end()) {
        return nullptr;
    }

    return it->second.get();
}

}
}
