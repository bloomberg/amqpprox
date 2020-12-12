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

#include <amqpprox_backend.h>
#include <amqpprox_backendset.h>
#include <amqpprox_datacenter.h>

#include <memory>
#include <utility>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::AffinityPartitionPolicy;
using Bloomberg::amqpprox::Backend;
using Bloomberg::amqpprox::BackendSet;
using Bloomberg::amqpprox::Datacenter;

TEST(AffinityPartitionPolicy, Breathing)
{
    Datacenter              datacenter;
    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    EXPECT_TRUE(true);
}

TEST(AffinityPartitionPolicy, PartitionPolicyNamedCorrectly)
{
    Datacenter              datacenter;
    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    EXPECT_EQ("datacenter-affinity", affinityPartitionPolicy.policyName());
}

TEST(AffinityPartitionPolicy, EmptyBackendSetUnchanged)
{
    // GIVEN
    Datacenter              datacenter;
    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    std::vector<BackendSet::Partition> partitions;
    std::shared_ptr<BackendSet>        backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitions, result->partitions());
}

TEST(AffinityPartitionPolicy, SingleBackendCorrectAffinity)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc1");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitions, result->partitions());
}

TEST(AffinityPartitionPolicy, SingleBackendIncorrectAffinity)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc2");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitions, result->partitions());
}

TEST(AffinityPartitionPolicy, TwoBackendAffinityOrder)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc1");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc2", "host", "ip", 100);

    // Starting in the correct order
    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);

    // Split into two partitions
    std::vector<BackendSet::Partition> partitionsResult(2);
    partitionsResult[0].push_back(&backend1);
    partitionsResult[1].push_back(&backend2);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitionsResult, result->partitions());
}

TEST(AffinityPartitionPolicy, TwoBackendNonAffinityOrder)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc1");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc2", "host", "ip", 100);

    // Starting in the incorrect order
    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend2);
    partitions[0].push_back(&backend1);

    // Split into two partitions
    std::vector<BackendSet::Partition> partitionsResult(2);
    partitionsResult[0].push_back(&backend1);
    partitionsResult[1].push_back(&backend2);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitionsResult, result->partitions());
}

TEST(AffinityPartitionPolicy, TwoPartitionsTwoBackendsSimilarAffinities)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc1");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc2", "host", "ip", 100);

    Backend backend3("name", "dc3", "host", "ip", 100);
    Backend backend4("name", "dc2", "host", "ip", 100);

    // Starting in the incorrect order
    std::vector<BackendSet::Partition> partitions(2);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);
    partitions[1].push_back(&backend3);
    partitions[1].push_back(&backend4);

    // Split into two partitions
    std::vector<BackendSet::Partition> partitionsResult(3);
    partitionsResult[0].push_back(&backend1);
    partitionsResult[1].push_back(&backend2);
    partitionsResult[2].push_back(&backend3);
    partitionsResult[2].push_back(&backend4);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitionsResult, result->partitions());
}

TEST(AffinityPartitionPolicy, TwoPartitionsTwoBackendsDistinctAffinities)
{
    // GIVEN
    Datacenter datacenter;
    datacenter.set("dc1");

    AffinityPartitionPolicy affinityPartitionPolicy(&datacenter);

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc2", "host", "ip", 100);

    Backend backend3("name", "dc3", "host", "ip", 100);
    Backend backend4("name", "dc1", "host", "ip", 100);

    // Starting in the incorrect order
    std::vector<BackendSet::Partition> partitions(2);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);
    partitions[1].push_back(&backend3);
    partitions[1].push_back(&backend4);

    // Split into two partitions
    std::vector<BackendSet::Partition> partitionsResult(4);
    partitionsResult[0].push_back(&backend1);
    partitionsResult[1].push_back(&backend2);
    partitionsResult[2].push_back(&backend4);
    partitionsResult[3].push_back(&backend3);

    std::shared_ptr<BackendSet> backendSet(new BackendSet(partitions));

    // WHEN
    std::shared_ptr<BackendSet> result =
        affinityPartitionPolicy.partition(backendSet);

    // THEN
    EXPECT_EQ(partitionsResult, result->partitions());
}
