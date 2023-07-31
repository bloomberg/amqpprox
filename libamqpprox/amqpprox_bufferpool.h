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
#ifndef BLOOMBERG_AMQPPROX_BUFFERPOOL
#define BLOOMBERG_AMQPPROX_BUFFERPOOL

#include <amqpprox_bufferhandle.h>
#include <amqpprox_buffersource.h>

#include <atomic>
#include <cstring>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Pool of buffers from a range of fixed size buffer sources.
 *
 * This component provides buffers either from heap allocation, or from a range
 * of `BufferSource` objects. The sizes the `BufferSource` objects provide is
 * parameterised at construction time.
 *
 * The pool is not threadsafe, apart from retrieving statistics on usage
 * through `getPoolStatistics`. The pool must have a lifetime that exceeds that
 * of all of the buffer handles given out by the pool.
 */
class BufferPool {
    std::vector<std::unique_ptr<BufferSource>> d_bufferSources;
    std::atomic<uint64_t>                      d_spillover;

  public:
    // TYPES
    /**
     * \brief The tuples are of the form: bufferSize, current allocation,
     * highest allocation (both in number of buffers).
     */
    using BufferAllocationStat = std::tuple<std::size_t, uint64_t, uint64_t>;

    // CREATORS
    /**
     * \brief Construct the pool with the given range of buffer sizes
     */
    explicit BufferPool(const std::vector<std::size_t> &buckets);

    BufferPool(const BufferPool &buffer)      = delete;
    BufferPool &operator=(const BufferPool &) = delete;
    BufferPool(BufferPool &&)                 = delete;
    BufferPool &operator=(BufferPool &&)      = delete;

    // MANIPULATORS
    /**
     * \brief Acquire a buffer of the given size `sz` and load it into the
     * provided `handle`. If the allocation fails `std::bad_alloc` will be
     * thrown
     */
    void acquireBuffer(BufferHandle *handle, std::size_t sz);

    // ACCESSORS
    /**
     * \brief Retrieve statistics on the current usage and highest usage for
     * each buffer size in the pool.
     */
    void getPoolStatistics(std::vector<BufferAllocationStat> *stats,
                           uint64_t                          *spilloverCount);
};

}
}

#endif
