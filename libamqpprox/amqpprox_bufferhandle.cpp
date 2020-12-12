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
#include <amqpprox_bufferhandle.h>

#include <amqpprox_buffersource.h>

namespace Bloomberg {
namespace amqpprox {

BufferHandle::BufferHandle(void *data, std::size_t size, BufferSource *source)
: d_data(data)
, d_size(size)
, d_source(source)
{
}

BufferHandle::BufferHandle()
: d_data(nullptr)
, d_size(0)
, d_source(nullptr)
{
}

BufferHandle::~BufferHandle()
{
    release();
}

void BufferHandle::assign(void *data, std::size_t size, BufferSource *source)
{
    release();
    d_data   = data;
    d_size   = size;
    d_source = source;
}

void BufferHandle::swap(BufferHandle &rhs)
{
    std::swap(d_data, rhs.d_data);
    std::swap(d_size, rhs.d_size);
    std::swap(d_source, rhs.d_source);
}

void BufferHandle::release()
{
    if (d_source) {
        d_source->release(d_data);
    }
    else {
        if (d_data) {
            delete[] static_cast<char *>(d_data);
        }
    }

    d_data   = nullptr;
    d_source = nullptr;
    d_size   = 0;
}

}
}
