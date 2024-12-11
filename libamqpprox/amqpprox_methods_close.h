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
#ifndef BLOOMBERG_AMQPPROX_METHODS_CLOSE
#define BLOOMBERG_AMQPPROX_METHODS_CLOSE

#include <boost/endian/arithmetic.hpp>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

/**
 * \brief Represents AMQP Connection CLOSE method
 */
class Close {
    boost::endian::big_uint16_t d_replyCode;
    std::string                 d_replyString;
    boost::endian::big_uint16_t d_classId;
    boost::endian::big_uint16_t d_methodId;

  public:
    Close();

    const uint16_t replyCode() const { return d_replyCode; }

    const std::string &replyString() const { return d_replyString; }

    const uint16_t classId() const { return d_classId; }

    const uint16_t methodId() const { return d_methodId; }

    inline void setReply(uint16_t           code,
                         const std::string &text,
                         uint16_t           classId  = 0,
                         uint16_t           methodId = 0)
    {
        d_replyCode   = code;
        d_replyString = text;

        // Close may be due to internal conditions or due to an exception
        // during handling a specific method. When a close is due to an
        // exception, the class and method id of the method which caused the
        // exception is provided.
        if (classId) {
            d_classId = classId;
        }
        if (methodId) {
            d_methodId = methodId;
        }
    }

    /**
     * \brief Decode specified buffer and copy the data into close method
     */
    static bool decode(Close *close, Buffer &buffer);

    /**
     * \brief Encode specified close method and write the data into buffer
     */
    static bool encode(Buffer &buffer, const Close &close);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 50; }
};

std::ostream &operator<<(std::ostream &os, const Close &closeMethod);

}
}
}

#endif
