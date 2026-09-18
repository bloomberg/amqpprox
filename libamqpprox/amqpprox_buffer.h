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
#ifndef BLOOMBERG_AMQPPROX_BUFFER
#define BLOOMBERG_AMQPPROX_BUFFER

#include <cassert>
#include <cstring>
#include <stdexcept>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Represents IO buffer
 */
class Buffer {
    char       *d_data;
    std::size_t d_length;
    std::size_t d_offset;

  public:
    Buffer()
    : d_data(nullptr)
    , d_length(0)
    , d_offset(0)
    {
    }

    constexpr Buffer(const void *start, std::size_t length)
    : d_data(static_cast<char *>(const_cast<void *>(start)))
    , d_length(length)
    , d_offset(0)
    {
    }

    Buffer(void *start, std::size_t length)
    : d_data(static_cast<char *>(start))
    , d_length(length)
    , d_offset(0)
    {
    }

    Buffer remaining() const { return Buffer(ptr(), available()); }

    Buffer currentData() const { return Buffer(originalPtr(), offset()); }

    Buffer consume(std::size_t size)
    {
        Buffer b(ptr(), size);
        skip(size);
        return b;
    }

    /**
     * \brief Read a `T` from the current offset and advance past it
     * \throws std::runtime_error if fewer than `sizeof(T)` bytes remain
     *
     * Defensive only: decoders should use `tryCopy`, which reports truncation
     * by return value. This check exists so that a caller using neither
     * `tryCopy` nor its own `available()` test still cannot read past the end.
     *
     * Safe if it does fire: every caller is a decoder running under
     * `Session::handleData`, which catches this and disconnects that one
     * session.
     */
    template <typename T>
    T copy()
    {
        if (sizeof(T) > available()) {
            throw std::runtime_error(
                "Buffer::copy: attempt to read past end of buffer");
        }

        T val;
        memcpy(&val, ptr(), sizeof(T));
        skip(sizeof(T));
        return val;
    }

    template <typename T>
    bool tryCopy(T *out)
    {
        if (sizeof(T) > available()) {
            return false;
        }

        *out = copy<T>();
        return true;
    }

    template <typename T>
    bool writeIn(const T &value)
    {
        if (sizeof(T) > available()) {
            return false;
        }

        memcpy(ptr(), &value, sizeof(T));
        skip(sizeof(T));

        return true;
    }

    bool writeIn(const Buffer &b)
    {
        return assign(b) ? (skip(b.available()), true) : false;
    }

    bool assign(Buffer value)
    {
        if (value.available() > available()) {
            return false;
        }

        memcpy(ptr(), value.ptr(), value.available());
        return true;
    }

    void skip(const std::size_t size)
    {
        // Safe as the code stands: throws reach `Session::handleData`, which
        // drops that one connection.
        //
        // Do not encode a field table outside `handleData` - a throw there
        // unwinds into `Server::run` and closes every session on the proxy.
        if (size > available()) {
            throw std::runtime_error(
                "Buffer::skip: attempt to move past end of buffer");
        }
        d_offset += size;
    }

    void seek(std::size_t offset)
    {
        assert(offset <= d_length);
        d_offset = offset;
    }

    constexpr const void *originalPtr() const { return d_data; }

    constexpr const void *ptr() const { return d_data + d_offset; }

    void *ptr() { return d_data + d_offset; }

    constexpr const void *end() const { return d_data + d_length; }

    constexpr std::size_t available() const { return d_length - d_offset; }

    constexpr std::size_t size() const { return d_length; }

    constexpr std::size_t offset() const { return d_offset; }

    bool equalContents(const Buffer &rhs) const;
};

bool operator==(const Buffer &b1, const Buffer &b2);
bool operator!=(const Buffer &b1, const Buffer &b2);

}
}

#endif
