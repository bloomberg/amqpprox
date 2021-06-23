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
#include <amqpprox_frame.h>

#include <amqpprox_constants.h>
#include <amqpprox_logging.h>

#include <iostream>
#include <stdexcept>

#include <boost/log/utility/manipulators/dump.hpp>

namespace logging = boost::log;

namespace Bloomberg {
namespace amqpprox {

std::size_t Frame::maxFrameSize = 150000;

Frame::Frame()
: type()
, channel()
, length()
, payload()
{
}

bool Frame::decode(Frame *      frame,
                   const void **endOfFrame,
                   std::size_t *remaining,
                   const void * buf,
                   std::size_t  bufferLen)
{
    if (bufferLen < frameOverhead()) {
        return false;
    }

    const uint8_t *buffer = static_cast<const uint8_t *>(buf);
    memcpy(&frame->type, &buffer[0], sizeof frame->type);
    memcpy(&frame->channel, &buffer[1], sizeof frame->channel);
    memcpy(&frame->length, &buffer[3], sizeof frame->length);

    if ((frameOverhead() + frame->length) > bufferLen) {
        return false;
    }

    std::size_t buffer_len = frameHeaderSize() + frame->length;
    if (buffer[buffer_len] != Constants::frameEnd()) {
        LOG_ERROR << "Frame: " << (int)frame->type << " " << frame->channel
                  << " " << frame->length;

        LOG_ERROR << "Full frame in log";
        LOG_WARN << "Frame: " << logging::dump(buffer, buffer_len + 1);

        throw std::runtime_error("Decode error");
    }

    frame->payload = buffer + frameHeaderSize();

    *remaining  = bufferLen - (frameOverhead() + frame->length);
    *endOfFrame = buffer + frame->length + frameOverhead();

    return true;
}

bool Frame::encode(void *output, std::size_t *writtenSize, const Frame &frame)
{
    if ((frame.length + frameOverhead()) > getMaxFrameSize()) {
        return false;
    }

    uint8_t *buffer = static_cast<uint8_t *>(output);
    memcpy(buffer, &frame.type, sizeof(frame.type));
    *writtenSize += sizeof(frame.type);
    memcpy(buffer + *writtenSize, &frame.channel, sizeof(frame.channel));
    *writtenSize += sizeof(frame.channel);
    memcpy(buffer + *writtenSize, &frame.length, sizeof(frame.length));
    *writtenSize += sizeof(frame.length);
    memcpy(buffer + *writtenSize, frame.payload, frame.length);
    *writtenSize += frame.length;
    buffer[*writtenSize] = 0xCE;
    *writtenSize += 1;
    return true;
}

std::size_t Frame::getMaxFrameSize()
{
    return maxFrameSize;
}

bool operator==(const Frame &f1, const Frame &f2)
{
    if (f1.type != f2.type) {
        return false;
    }

    if (f1.channel != f2.channel) {
        return false;
    }

    if (f1.length != f2.length) {
        return false;
    }

    return 0 == memcmp(f1.payload, f2.payload, f1.length);
}

bool operator!=(const Frame &f1, const Frame &f2)
{
    return !(f1 == f2);
}

}
}
