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
#include <amqpprox_methods_start.h>

#include <amqpprox_buffer.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_fieldvalue.h>
#include <amqpprox_frame.h>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::Buffer;
using Bloomberg::amqpprox::FieldTable;
using Bloomberg::amqpprox::FieldValue;
using Bloomberg::amqpprox::Frame;
using Bloomberg::amqpprox::methods::Start;

TEST(Methods_Start, Start_Breathing)
{
    Start s;
}

TEST(Methods_Start, Start_Encode_Decode)
{
    FieldTable ft;
    FieldValue fv('s', std::string("ShortString"));
    ft.pushField("foo", fv);
    Start startMethod(0, 9, ft, {"PLAIN", "AMQPLAIN"}, {"en-US"});

    std::vector<uint8_t> encodeBuffer(Frame::getMaxFrameSize());
    Buffer               encodeBuf(encodeBuffer.data(), encodeBuffer.size());
    bool                 encoded = Start::encode(encodeBuf, startMethod);
    EXPECT_TRUE(encoded);

    Buffer encodedData = encodeBuf.currentData();
    Start  decodedStartMethod;
    bool   decoded = Start::decode(&decodedStartMethod, encodedData);
    EXPECT_TRUE(decoded);

    EXPECT_EQ(startMethod, decodedStartMethod);
}

TEST(Methods_Start, Start_Decode_Fail)
{
    FieldTable ft;
    FieldValue fv('s', std::string("ShortString"));
    ft.pushField("foo", fv);
    Start startMethod(0, 9, ft, {"PLAIN", "AMQPLAIN"}, {"en-US"});

    std::vector<uint8_t> encodeBuffer(Frame::getMaxFrameSize());
    Buffer               encodeBuf(encodeBuffer.data(), encodeBuffer.size());
    bool                 encoded = Start::encode(encodeBuf, startMethod);
    EXPECT_TRUE(encoded);

    Buffer encodedData = encodeBuf.currentData();
    auto len = encodedData.size();
    for (int i = len - 1; i > 0; --i) {
        Buffer testData(encodedData.originalPtr(), i);
        Start  decodedStartMethod;
        bool   decoded = Start::decode(&decodedStartMethod, testData);
        EXPECT_FALSE(decoded);
    }
}
