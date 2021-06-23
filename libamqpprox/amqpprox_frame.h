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
#ifndef BLOOMBERG_AMQPPROX_FRAME
#define BLOOMBERG_AMQPPROX_FRAME

#include <boost/endian/arithmetic.hpp>

namespace Bloomberg {
namespace amqpprox {

class Frame {
    static std::size_t maxFrameSize;

  public:
    boost::endian::big_uint8_t  type;
    boost::endian::big_uint16_t channel;
    boost::endian::big_uint32_t length;
    const void *                payload;

    Frame();

    static bool decode(Frame *      frame,
                       const void **endOfFrame,
                       std::size_t *remaining,
                       const void * buffer,
                       std::size_t  bufferLen);

    static bool
    encode(void *output, std::size_t *writtenSize, const Frame &frame);

    static std::size_t getMaxFrameSize();
    ///< Return the maximum frame size we support

    static constexpr std::size_t frameOverhead();
    ///< Return the overhead of the framing, beyond the payload size

    static constexpr std::size_t frameHeaderSize();
    ///< Return the size of the frame header
};

constexpr std::size_t Frame::frameOverhead()
{
    return frameHeaderSize() + 1;  // frameHeaderSize() + sentinel value
}

constexpr std::size_t Frame::frameHeaderSize()
{
    return sizeof(type) + sizeof(channel) + sizeof(length);
}

bool operator==(const Frame &f1, const Frame &f2);
bool operator!=(const Frame &f1, const Frame &f2);

}
}

#endif
