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
#ifndef BLOOMBERG_AMQPPROX_BUFFERSOURCE
#define BLOOMBERG_AMQPPROX_BUFFERSOURCE

#include <atomic>

#include <boost/pool/pool.hpp>

namespace Bloomberg {
namespace amqpprox {

/* \brief Source of a pool of buffers of a particular size
 *
 * This class provides access to a pool of buffers of a fixed size,
 * parameterized in the constructor. This class is NOT thread safe in general,
 * it has one thread-safe method `allocationStats` for acquiring statistics and
 * the `bufferSize` method is also threadsafe.
 */
class BufferSource {
    boost::pool<>         d_pool;
    std::atomic<uint64_t> d_allocationCount;
    std::atomic<uint64_t> d_deallocationCount;
    std::atomic<uint64_t> d_highWater;
    std::size_t           d_allocatedBufferSize;

  public:
    // CREATORS
    explicit BufferSource(std::size_t bufferSize);

    BufferSource(const BufferSource &) = delete;
    BufferSource &operator=(const BufferSource &) = delete;

    // MANIPULATORS
    /**
     * \brief Release the buffer `data` passed in
     */
    void release(void *data);

    /**
     * \brief Acquire a buffer of the `bufferSize`
     */
    void *acquire();

    // ACCESSORS
    /**
     * \return Size of the buffers managed by this component, this is a
     * threadsafe method to call
     */
    std::size_t bufferSize() const;

    /**
     * \brief Retrieve the current allocation statistics, this is a threadsafe
     * method to call
     * \param allocationCount Allocation count
     * \param deallocationCount Deallocation count
     * \param highwaterMark High-water mark
     */
    void allocationStats(uint64_t *allocationCount,
                         uint64_t *deallocationCount,
                         uint64_t *highwaterMark) const;
};

}
}

#endif
