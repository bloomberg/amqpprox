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
#include <amqpprox_backendset.h>

#include <utility>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

// CREATORS
BackendSet::BackendSet(std::vector<BackendSet::Partition> partitions)
: d_partitions(std::move(partitions))
, d_markers(d_partitions.size(), 0)
{
}

// MANIPULATORS
uint64_t BackendSet::markPartition(uint64_t partitionId)
{
    if (partitionId > d_markers.size()) {
        return 0;
    }

    return ++d_markers[partitionId];
}

}
}
