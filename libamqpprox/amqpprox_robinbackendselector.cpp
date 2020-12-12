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
#include <amqpprox_backendselector.h>
#include <amqpprox_backendset.h>

#include <vector>

namespace Bloomberg {
namespace amqpprox {

namespace {

const std::string SELECTOR_NAME("round-robin");

}

const Backend *
RobinBackendSelector::select(BackendSet *                 backendSet,
                             const std::vector<uint64_t> &markers,
                             uint64_t                     retryCount) const
{
    uint64_t retry = retryCount;
    uint64_t i     = 0;

    for (const auto &marker : markers) {
        uint64_t partitionSize = backendSet->partitions()[i].size();

        if (retry >= partitionSize) {
            retry -= partitionSize;
        }
        else {
            uint64_t point = (marker + retry) % partitionSize;
            backendSet->markPartition(i);
            return backendSet->partitions()[i][point];
        }

        ++i;
    }

    return nullptr;
}

const std::string &RobinBackendSelector::selectorName() const
{
    return SELECTOR_NAME;
}

}
}
