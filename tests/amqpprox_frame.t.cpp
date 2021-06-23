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

#include <cstddef>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(Frame, Equality)
{
    Frame f1;
    f1.type    = 8;
    f1.channel = 0;
    f1.length  = 0;
    f1.payload = nullptr;

    EXPECT_EQ(f1, f1);
}

TEST(Frame, Equality_Mismatch)
{
    Frame f1;
    f1.type    = 8;
    f1.channel = 0;
    f1.length  = 0;
    f1.payload = nullptr;

    Frame f2(f1);
    EXPECT_EQ(f1, f2);

    f2.type = 7;

    EXPECT_NE(f1, f2);

    f2.type    = 8;
    f2.channel = 1;

    EXPECT_NE(f1, f2);

    f2.channel = 0;
    f2.length  = 1;

    EXPECT_NE(f1, f2);

    f2.length = 0;

    EXPECT_EQ(f1, f2);
}

TEST(Frame, HeartbeatFrame)
{
    Frame f1;
    f1.type    = 8;
    f1.channel = 0;
    f1.length  = 0;
    f1.payload = nullptr;

    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    std::size_t          outputSize = 0;
    bool encoded = Frame::encode(buffer.data(), &outputSize, f1);

    EXPECT_EQ(outputSize, 8);
    EXPECT_TRUE(encoded);
    int ret = memcmp(buffer.data(), "\x08\x00\x00\x00\x00\x00\x00\xCE", 8);
    EXPECT_EQ(ret, 0);

    const void *endOfFrame;
    std::size_t remaining = 111111111;
    Frame       f2;
    bool        decoded =
        Frame::decode(&f2, &endOfFrame, &remaining, buffer.data(), outputSize);
    EXPECT_TRUE(decoded);
    EXPECT_EQ(remaining, 0);
    EXPECT_EQ(f1, f2);
}

TEST(Frame, CantFitHeartBeat)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame  = nullptr;
    std::size_t          remaining   = 11111;
    const char *         almostFrame = "\x08\x00\x00\x00\x00\x00\x00";
    buffer.assign(almostFrame, almostFrame + 7);
    Frame f1;
    bool  decodable =
        Frame::decode(&f1, &endOfFrame, &remaining, buffer.data(), 7);

    EXPECT_FALSE(decodable);
}

TEST(Frame, CantFitHeartBeatBy2)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame  = nullptr;
    std::size_t          remaining   = 11111;
    const char *         almostFrame = "\x08\x00\x00\x00\x00\x00";
    buffer.assign(almostFrame, almostFrame + 6);
    Frame f1;
    bool  decodable =
        Frame::decode(&f1, &endOfFrame, &remaining, buffer.data(), 6);

    EXPECT_FALSE(decodable);
}

TEST(Frame, OverSpillHeartBeat)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame  = nullptr;
    std::size_t          remaining   = 11111;
    const char *         almostFrame = "\x08\x00\x00\x00\x00\x00\x00\xCE\xFF";
    buffer.assign(almostFrame, almostFrame + 9);
    Frame f1;
    bool  decodable =
        Frame::decode(&f1, &endOfFrame, &remaining, buffer.data(), 9);

    EXPECT_TRUE(decodable);
    EXPECT_EQ(f1.type, 8);
    EXPECT_EQ(f1.channel, 0);
    EXPECT_EQ(f1.length, 0);
    EXPECT_EQ(remaining, 1);
    EXPECT_EQ(endOfFrame, buffer.data() + 8);
}

TEST(Frame, OverSpillFakePayload)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame = nullptr;
    std::size_t          remaining  = 11111;
    const char *almostFrame = "\x08\x00\x01\x00\x00\x00\x02\xFF\xFF\xCE\xFF";
    buffer.assign(almostFrame, almostFrame + 11);
    Frame f1;
    bool  decodable =
        Frame::decode(&f1, &endOfFrame, &remaining, buffer.data(), 11);

    EXPECT_TRUE(decodable);
    EXPECT_EQ(f1.type, 8);
    EXPECT_EQ(f1.channel, 1);
    EXPECT_EQ(f1.length, 2);
    EXPECT_EQ(remaining, 1);
    EXPECT_EQ(endOfFrame, buffer.data() + 10);
    int payloadCmp = memcmp(f1.payload, "\xFF\xFF", 2);
    EXPECT_EQ(0, payloadCmp);
}

TEST(Frame, OneByteTooLittle)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame = nullptr;
    std::size_t          remaining  = 11111;
    const char *almostFrame = "\x08\x00\x01\x00\x00\x00\x02\xFF\xFF\xCE\xFF";
    buffer.assign(almostFrame, almostFrame + 11);
    Frame f1;
    bool  decodable = Frame::decode(&f1,
                                   &endOfFrame,
                                   &remaining,
                                   buffer.data(),
                                   9);  // cut the buffer short

    EXPECT_FALSE(decodable);
    EXPECT_EQ(remaining, 11111);
    EXPECT_EQ(endOfFrame, nullptr);
}

TEST(Frame, BadSentinelChar)
{
    std::vector<uint8_t> buffer(Frame::getMaxFrameSize());
    const void *         endOfFrame = nullptr;
    std::size_t          remaining  = 11111;
    const char *almostFrame = "\x08\x00\x01\x00\x00\x00\x02\xFF\xFF\xCD\xFF";
    //   ^^
    buffer.assign(almostFrame, almostFrame + 11);
    Frame f1;
    EXPECT_THROW(
        Frame::decode(&f1, &endOfFrame, &remaining, buffer.data(), 11),
        std::runtime_error);

    EXPECT_EQ(remaining, 11111);
    EXPECT_EQ(endOfFrame, nullptr);
}

TEST(Frame, Cant_Encode_Payload_Too_Large)
{
    Frame f1;
    f1.type    = 8;
    f1.channel = 0;
    f1.length  = Frame::getMaxFrameSize() - Frame::frameOverhead() + 1;
    f1.payload = nullptr;

    void *      output;
    std::size_t sz = 0;
    EXPECT_FALSE(Frame::encode(output, &sz, f1));
}
