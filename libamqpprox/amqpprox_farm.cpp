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
#include <amqpprox_farm.h>

#include <amqpprox_backendselector.h>
#include <amqpprox_backendset.h>
#include <amqpprox_backendstore.h>
#include <amqpprox_partitionpolicy.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

// CREATORS
Farm::Farm(const std::string              &name,
           const std::vector<std::string> &members,
           BackendStore                   *backendStore,
           BackendSelector                *backendSelector)
: d_name(name)
, d_backendMembers()
, d_backendStore_p(backendStore)
, d_backendSelector_p(backendSelector)
, d_partitionPolicies()
, d_backendSet()
, d_mutex()
{
    std::copy(members.cbegin(),
              members.cend(),
              std::inserter(d_backendMembers, d_backendMembers.end()));

    repartition();
}

// MANIPULATORS
void Farm::addMember(const std::string &backend)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_backendMembers.insert(backend);
    doRepartitionWhileLocked(lg);
}

void Farm::removeMember(const std::string &backend)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_backendMembers.erase(backend);
    doRepartitionWhileLocked(lg);
}

void Farm::setBackendSelector(BackendSelector *selector)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_backendSelector_p = selector;
}

void Farm::addPartitionPolicy(PartitionPolicy *partitionPolicy)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_partitionPolicies.push_back(partitionPolicy);
    doRepartitionWhileLocked(lg);
}

void Farm::repartition()
{
    std::lock_guard<std::mutex> lg(d_mutex);
    doRepartitionWhileLocked(lg);
}

void Farm::doRepartitionWhileLocked(std::lock_guard<std::mutex> & /* guard */)
{
    std::vector<BackendSet::Partition> partitions(1);
    BackendSet::Partition             &initialPartition = partitions[0];

    // Create the initial partition
    for (const auto &member : d_backendMembers) {
        initialPartition.push_back(d_backendStore_p->lookup(member));
    }

    auto newSet = std::make_shared<BackendSet>(std::move(partitions));

    // Apply all partition policies
    for (auto policy : d_partitionPolicies) {
        newSet = policy->partition(newSet);
    }

    d_backendSet = newSet;
}

// ACCESSORS
const std::string &Farm::name() const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    return d_name;
}

void Farm::members(std::vector<std::string> *members) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    std::copy(d_backendMembers.cbegin(),
              d_backendMembers.cend(),
              std::back_inserter(*members));
}

std::shared_ptr<BackendSet> Farm::backendSet() const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    return d_backendSet;
}

BackendSelector *Farm::backendSelector() const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    return d_backendSelector_p;
}

std::ostream &operator<<(std::ostream &os, const Farm &farm)
{
    os << farm.name();
    os << ": ";

    std::vector<std::string> members;
    farm.members(&members);
    for (const auto &member : members) {
        os << member << " ";
    }

    return os;
}

}
}
