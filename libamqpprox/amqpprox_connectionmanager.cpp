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
#include <amqpprox_connectionmanager.h>

#include <amqpprox_backend.h>
#include <amqpprox_backendselector.h>
#include <amqpprox_backendset.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

// CREATORS
ConnectionManager::ConnectionManager(std::shared_ptr<BackendSet> backendSet,
                                     BackendSelector *backendSelector)
: d_backendSet(std::move(backendSet))
, d_markerSnapshot(d_backendSet->markers())
, d_backendSelector_p(backendSelector)
{
}

const Backend *ConnectionManager::getConnection(uint64_t retryCount) const
{
    if (d_backendSelector_p) {
        return d_backendSelector_p->select(
            d_backendSet.get(), d_markerSnapshot, retryCount);
    }
    else {
        // The ConnectionManager must handle the special case where a vhost has
        // been mapped directly to a Backend, and no Farm or BackendSelector is
        // involved. In this case, the BackendSet will have one partition
        // containing one element that should be returned once, and not
        // retried.

        if (retryCount > 0) {
            return nullptr;
        }

        const std::vector<BackendSet::Partition> &partitions =
            d_backendSet->partitions();

        if (partitions.size() == 0) {
            return nullptr;
        }

        const std::vector<const Backend *> &backends = partitions[0];
        if (backends.size() == 0) {
            return nullptr;
        }

        return backends[0];
    }

    return nullptr;
}

}
}
