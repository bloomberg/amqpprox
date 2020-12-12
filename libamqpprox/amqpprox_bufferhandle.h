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
#ifndef BLOOMBERG_AMQPPROX_BUFFERHANDLE
#define BLOOMBERG_AMQPPROX_BUFFERHANDLE

#include <iostream>
#include <memory>

namespace Bloomberg {
namespace amqpprox {

class BufferSource;

/* \brief Handle to own a sized buffer
 *
 * This component provides a simple scoped handle to own a buffer which is
 * either from the heap or a provided 'BufferSource'. At this time the
 * 'BufferSource' is not a virtual dispatch, as its not needed to be, but could
 * be switched at a later time with minimal changes.
 */
class BufferHandle {
    void *        d_data;
    std::size_t   d_size;
    BufferSource *d_source;

  public:
    // CREATORS
    BufferHandle(void *data, std::size_t size, BufferSource *source);
    ///< Initialise a handle with the prescribed data pointer, size and
    ///< providence `BufferSource`.

    BufferHandle();
    ///< Initialise an unset handle

    BufferHandle(const BufferHandle &buffer) = delete;
    BufferHandle &operator=(const BufferHandle &) = delete;
    BufferHandle(BufferHandle &&)                 = delete;
    BufferHandle &operator=(BufferHandle &&) = delete;

    ~BufferHandle();
    ///< Release the held data, if the buffer handle is set

    // MANIPULATORS
    void assign(void *data, std::size_t size, BufferSource *source);
    ///< Set the `data`, `size` and `source` of this handle, releasing any
    ///< prior held data before.

    void swap(BufferHandle &rhs);
    ///< Swap the contents of this handle with the provided `rhs`

    void release();
    ///< Release the data member either by using the `BufferSource` it came
    ///< from, or array delete. Resets the pointers back to null, and size
    ///< to zero.

    // ACCESSORS
    inline void *data();
    ///< Return the raw data pointer managed by this `BufferHandle`, or
    ///< nullptr if unset.

    inline std::size_t size() const;
    ///< Return the size of the managed data section, or 0 if unset.

    inline BufferSource *source() const;
    ///< Return the `BufferSource` that was the providence of the data
    ///< buffer, or nullptr if there is no source, or the handle is unset
};

inline void *BufferHandle::data()
{
    return d_data;
}

inline std::size_t BufferHandle::size() const
{
    return d_size;
}

inline BufferSource *BufferHandle::source() const
{
    return d_source;
}

}
}

#endif
