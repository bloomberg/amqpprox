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
#ifndef BLOOMBERG_AMQPPROX_METHOD
#define BLOOMBERG_AMQPPROX_METHOD

#include <amqpprox_buffer.h>

#include <boost/endian/arithmetic.hpp>

namespace Bloomberg {
namespace amqpprox {

class Method {
  public:
    boost::endian::big_uint16_t classType;
    boost::endian::big_uint16_t methodType;
    const void *                payload;
    std::size_t                 length;

    static bool
    decode(Method *method, const void *buffer, std::size_t bufferLen);

    template <typename T>
    static bool encode(Buffer &buffer, const T &method);
};

template <typename T>
bool Method::encode(Buffer &buffer, const T &method)
{
    return buffer.writeIn<boost::endian::big_uint16_t>(T::classType()) &&
           buffer.writeIn<boost::endian::big_uint16_t>(T::methodType()) &&
           T::encode(buffer, method);
}

}
}

#endif
