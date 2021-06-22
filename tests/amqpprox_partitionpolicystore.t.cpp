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


#include <amqpprox_datacenter.h>
#include <amqpprox_partitionpolicystore.h>
#include <amqpprox_affinitypartitionpolicy.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(PartitionPolicyStore, Breathing) {
    PartitionPolicyStore store;

    Datacenter datacenter;
    datacenter.set("LONDON");
    std::unique_ptr<PartitionPolicy> policy(new AffinityPartitionPolicy(&datacenter));

    store.addPolicy(std::move(policy));
    EXPECT_NE(store.getPolicy("datacenter-affinity"), nullptr);
    EXPECT_EQ(store.getPolicy("non-existing"), nullptr);
}

