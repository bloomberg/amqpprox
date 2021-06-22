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
#ifndef BLOOMBERG_AMQPPROX_FRAME
#define BLOOMBERG_AMQPPROX_FRAME

#include <boost/endian/arithmetic.hpp>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Representation of an AMQP 0-9-1 Frame
 */
class Frame {
    static std::size_t maxFrameSize;

  public:
    boost::endian::big_uint8_t  type;
    boost::endian::big_uint16_t channel;
    boost::endian::big_uint32_t length;
    const void *                payload;

    Frame();

    /**
     * \brief Decode a frame from a buffer per spec:
     * <type> <channel> <size> <payload> <end of frame byte>
     *
     * \throw  will throw a runtime_error if the frame is malformed e.g. no
     * frame end byte
     *
     * \param frame points to the `Frame` object to be populated
     *
     * \param endOfFrame populates the pointer pointed to with the first byte
     * beyond in the buffer after the decoded frame (useful for the next
     * decode)
     *
     * \param remaining points to the variable to be populated with the
     * number of bytes remaining in the buffer
     *
     * \param buffer pointer to the beginning of the byte stream to decode
     * from
     *
     * \param bufferLen the number of readable bytes contained in the buffer
     *
     * \returns true if a frame was successfully read, false if there was not
     * enough bytes in the buffer to decode the frame
     */
    static bool decode(Frame *      frame,
                       const void **endOfFrame,
                       std::size_t *remaining,
                       const void * buffer,
                       std::size_t  bufferLen);

    /**
     * \brief Encode a frame into a buffer per spec:
     * <type> <channel> <size> <payload> <end of frame byte>
     *
     * \param output a buffer of size >= maximum frame size, passing a buffer
     * smaller than the serialized size of the frame yields undefined behaviour
     *
     * \param writtenSize populated with the serialized frame length in bytes
     *
     * \param frame reference to a the frame to be encoded
     *
     * \returns true if a frame was successfully written to the buffer, false
     * if the frame was too large
     */
    static bool
    encode(void *output, std::size_t *writtenSize, const Frame &frame);

    /**
     * \returns the maximum frame size we support in bytes
     */
    static std::size_t getMaxFrameSize();

    /**
     * \returns the overhead of the framing, beyond the payload size in bytes
     */
    static constexpr std::size_t frameOverhead();

    /**
     * \returns the size of the frame header in bytes
     */
    static constexpr std::size_t frameHeaderSize();
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
