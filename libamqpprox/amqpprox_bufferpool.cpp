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
#include <amqpprox_bufferpool.h>

#include <algorithm>
#include <tuple>

namespace Bloomberg {
namespace amqpprox {

BufferPool::BufferPool(const std::vector<std::size_t> &bucketsIn)
: d_bufferSources()
, d_spillover(0)
{
    std::vector<std::size_t> buckets(bucketsIn);
    std::sort(begin(buckets), end(buckets));
    d_bufferSources.reserve(buckets.size());

    for (const auto &b : buckets) {
        d_bufferSources.emplace_back(new BufferSource(b));
    }
}

void BufferPool::acquireBuffer(BufferHandle *handle, std::size_t sz)
{
    for (auto &source : d_bufferSources) {
        if (sz <= source->bufferSize()) {
            handle->assign(source->acquire(), sz, source.get());
            return;
        }
    }

    handle->assign(new char[sz], sz, nullptr);
    d_spillover.fetch_add(1, std::memory_order_relaxed);
    return;
}

void BufferPool::getPoolStatistics(std::vector<BufferAllocationStat> *stats,
                                   uint64_t *spilloverCount)
{
    for (const auto &source : d_bufferSources) {
        uint64_t    allocCount, deallocCount, highwaterMark;
        uint64_t    currentAllocation = 0;
        std::size_t bufSize           = source->bufferSize();

        source->allocationStats(&allocCount, &deallocCount, &highwaterMark);

        if (allocCount > deallocCount) {
            currentAllocation = allocCount - deallocCount;
        }

        stats->push_back(
            std::make_tuple(bufSize, currentAllocation, highwaterMark));
    }

    *spilloverCount = d_spillover.load(std::memory_order_relaxed);
}

}
}
