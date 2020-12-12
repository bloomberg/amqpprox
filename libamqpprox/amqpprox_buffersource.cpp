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
#include <amqpprox_buffersource.h>

#include <algorithm>

namespace Bloomberg {
namespace amqpprox {

BufferSource::BufferSource(std::size_t bufferSize)
: d_pool(bufferSize)
, d_allocationCount(0)
, d_deallocationCount(0)
, d_highWater(0)
, d_allocatedBufferSize(bufferSize)
{
}

void BufferSource::release(void *data)
{
    d_deallocationCount.fetch_add(1, std::memory_order_relaxed);
    d_pool.free(data);
}

void *BufferSource::acquire()
{
    d_allocationCount.fetch_add(1, std::memory_order_relaxed);
    uint64_t allocCount = d_allocationCount.load(std::memory_order_relaxed);
    uint64_t deallocCount =
        d_deallocationCount.load(std::memory_order_relaxed);

    // This line is technically racy; however, the atomics are used only for
    // statistics purposes and the writes necessarily have to only happen from
    // the same thread because of the unprotected `d_pool` access.
    d_highWater.store(std::max(d_highWater.load(std::memory_order_relaxed),
                               allocCount - deallocCount),
                      std::memory_order_relaxed);

    auto buf = d_pool.malloc();
    if (!buf) {
        throw std::bad_alloc();
    }
    return buf;
}

std::size_t BufferSource::bufferSize() const
{
    return d_allocatedBufferSize;
}

void BufferSource::allocationStats(uint64_t *allocationCount,
                                   uint64_t *deallocationCount,
                                   uint64_t *highwaterMark) const
{
    *allocationCount   = d_allocationCount.load(std::memory_order_relaxed);
    *deallocationCount = d_deallocationCount.load(std::memory_order_relaxed);
    *highwaterMark     = d_highWater.load(std::memory_order_relaxed);
}

}
}
