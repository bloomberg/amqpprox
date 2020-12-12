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
#include <amqpprox_robinbackendselector.h>

#include <amqpprox_backend.h>
#include <amqpprox_backendset.h>
#include <amqpprox_datacenter.h>

#include <memory>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::Backend;
using Bloomberg::amqpprox::BackendSet;
using Bloomberg::amqpprox::RobinBackendSelector;

namespace {

void testRetrySelection(const RobinBackendSelector &        selector,
                        BackendSet *                        backendSet,
                        const std::vector<uint64_t> &       markers,
                        uint64_t                            maxRetries,
                        const std::vector<const Backend *> &expectedOrder)
{
    for (uint64_t i = 0; i < maxRetries; ++i) {
        const Backend *result = selector.select(backendSet, markers, i);

        std::ostringstream oss;
        oss << "Executing expected order " << i;
        SCOPED_TRACE(oss.str());

        EXPECT_EQ(expectedOrder[i], result);
    }
}

}

TEST(RobinBackendSelector, Breathing)
{
    RobinBackendSelector robinBackendSelector;

    EXPECT_TRUE(true);
}

TEST(RobinBackendSelector, PartitionPolicyNamedCorrectly)
{
    RobinBackendSelector robinBackendSelector;

    EXPECT_EQ("round-robin", robinBackendSelector.selectorName());
}

TEST(RobinBackendSelector, SelectNullValueWhenNoneAvailable)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    std::vector<BackendSet::Partition> partitions;
    BackendSet                         backendSet(partitions);

    std::vector<uint64_t> markers;

    uint64_t retryCount = 0;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(nullptr, result);
}

TEST(RobinBackendSelector, SelectOnlyValueImmediately)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(0);

    uint64_t retryCount = 0;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(&backend1, result);
}

TEST(RobinBackendSelector, SelectOnlyValueByMarker)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(11);

    uint64_t retryCount = 0;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(&backend1, result);
}

TEST(RobinBackendSelector, FailWhenRetryCountExceedsAvailableValues)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(0);

    uint64_t retryCount = 2;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(nullptr, result);
}

TEST(RobinBackendSelector, SelectSingleValueSinglePartitionByMarker)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(3);

    uint64_t retryCount = 0;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(&backend2, result);
}

TEST(RobinBackendSelector, SelectSingleValueSinglePartitionByRetryCount)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(0);

    uint64_t retryCount = 1;

    // WHEN
    const Backend *result =
        robinBackendSelector.select(&backendSet, markers, retryCount);

    // THEN
    EXPECT_EQ(&backend2, result);
}

TEST(RobinBackendSelector,
     SelectSingleValueSinglePartitionByMarkerAndRetryCount)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc1", "host", "ip", 100);
    Backend backend3("name", "dc1", "host", "ip", 100);
    Backend backend4("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(1);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);
    partitions[0].push_back(&backend3);
    partitions[0].push_back(&backend4);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(2);

    // WHEN
    std::vector<const Backend *> expectedResult;
    expectedResult.push_back(&backend3);
    expectedResult.push_back(&backend4);
    expectedResult.push_back(&backend1);
    expectedResult.push_back(&backend2);

    // THEN
    testRetrySelection(
        robinBackendSelector, &backendSet, markers, 4, expectedResult);
}

TEST(RobinBackendSelector,
     SelectSingleValueMultiPartitionByMarkerAndRetryCount)
{
    // GIVEN
    RobinBackendSelector robinBackendSelector;

    Backend backend1("name", "dc1", "host", "ip", 100);
    Backend backend2("name", "dc1", "host", "ip", 100);

    Backend backend3("name", "dc1", "host", "ip", 100);
    Backend backend4("name", "dc1", "host", "ip", 100);
    Backend backend5("name", "dc1", "host", "ip", 100);

    std::vector<BackendSet::Partition> partitions(2);
    partitions[0].push_back(&backend1);
    partitions[0].push_back(&backend2);

    partitions[1].push_back(&backend3);
    partitions[1].push_back(&backend4);
    partitions[1].push_back(&backend5);

    BackendSet backendSet(partitions);

    std::vector<uint64_t> markers;
    markers.push_back(9);  // backend2
    markers.push_back(2);  // backend5

    // WHEN
    std::vector<const Backend *> expectedResult;
    expectedResult.push_back(&backend2);
    expectedResult.push_back(&backend1);
    expectedResult.push_back(&backend5);
    expectedResult.push_back(&backend3);
    expectedResult.push_back(&backend4);

    // THEN
    testRetrySelection(
        robinBackendSelector, &backendSet, markers, 5, expectedResult);
}
