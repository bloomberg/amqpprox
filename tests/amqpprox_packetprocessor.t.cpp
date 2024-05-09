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
#include <amqpprox_packetprocessor.h>

#include <amqpprox_buffer.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_connector.h>
#include <amqpprox_dnshostnamemapper.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_flowtype.h>
#include <amqpprox_frame.h>
#include <amqpprox_sessionstate.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Bloomberg {
namespace amqpprox {

using namespace ::testing;

class PacketProcessorTest : public ::testing::Test {
  public:
    PacketProcessorTest()
    : d_pool({32,
              64,
              128,
              256,
              512,
              1024,
              4096,
              16384,
              32768,
              65536,
              Frame::getMaxFrameSize()})
    , d_eventSource()
    , d_mapper(std::make_shared<DNSHostnameMapper>())
    , d_sessionState(d_mapper)
    , d_connector(&d_sessionState, &d_eventSource, &d_pool, "hostname")
    {
    }

    BufferPool                         d_pool;
    EventSource                        d_eventSource;
    std::shared_ptr<DNSHostnameMapper> d_mapper;

    SessionState d_sessionState;
    Connector    d_connector;
};

TEST_F(PacketProcessorTest, Breathing)
{
    PacketProcessor processor(d_sessionState, d_connector);
    EXPECT_THAT(d_connector.state(),
                Eq(Connector::State::AWAITING_PROTOCOL_HEADER));
}

TEST_F(PacketProcessorTest, HeaderPassedInOne)
{
    // If we get a full AMQP header the Connector should move state
    PacketProcessor processor(d_sessionState, d_connector);

    Buffer buffer((const void *)Constants::protocolHeader(),
                  Constants::protocolHeaderLength());
    buffer.seek(Constants::protocolHeaderLength());

    processor.process(FlowType::INGRESS, buffer);

    EXPECT_THAT(processor.remaining().size(), Eq(0));
    EXPECT_THAT(d_connector.state(), Eq(Connector::State::START_SENT));
}

TEST_F(PacketProcessorTest, MalformedStartOk)
{
    PacketProcessor processor(d_sessionState, d_connector);

    // Header
    {
        Buffer buffer((const void *)Constants::protocolHeader(),
                      Constants::protocolHeaderLength());
        buffer.seek(Constants::protocolHeaderLength());

        processor.process(FlowType::INGRESS, buffer);
    }

    {
        const uint8_t data[] = {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce};
        size_t size = sizeof(data) / sizeof(*data);
        Buffer buffer(data, size);
        buffer.seek(size);

        processor.process(FlowType::INGRESS, buffer);
    }
}

TEST_F(PacketProcessorTest, IncompleteHeader)
{
    // If we give the PacketProcessor less than the full header, we shouldn't
    // move on yet
    const size_t halfHeader = 4;

    Buffer buffer((const void *)Constants::protocolHeader(), halfHeader);
    buffer.seek(halfHeader);

    PacketProcessor processor(d_sessionState, d_connector);
    processor.process(FlowType::INGRESS, buffer);

    Buffer remaining = processor.remaining();

    EXPECT_THAT(remaining.size(), Eq(4));
    EXPECT_TRUE(remaining.equalContents(buffer));

    EXPECT_THAT(d_connector.state(),
                Eq(Connector::State::AWAITING_PROTOCOL_HEADER));
}

TEST_F(PacketProcessorTest, IncompleteHeaderInLargerBuffer)
{
    // Setup a 1024 byte buffer with 4 bytes written to it
    const size_t         bufferSize = 1024;
    std::vector<uint8_t> data;
    data.resize(bufferSize);

    const size_t halfHeader = 4;
    Buffer       buffer((const void *)data.data(), bufferSize);
    memcpy(buffer.ptr(), Constants::protocolHeader(), halfHeader);
    buffer.seek(halfHeader);

    // Process the buffer
    PacketProcessor processor(d_sessionState, d_connector);
    processor.process(FlowType::INGRESS, buffer);

    // Expect that the data marked as remaining contains just the half header
    Buffer expectedBuffer((const void *)Constants::protocolHeader(),
                          halfHeader);
    expectedBuffer.seek(halfHeader);

    Buffer remaining = processor.remaining();
    EXPECT_THAT(remaining.size(), Eq(4));
    EXPECT_TRUE(remaining.equalContents(expectedBuffer));

    // And that the Connector is still awaiting a full protocol header
    EXPECT_THAT(d_connector.state(),
                Eq(Connector::State::AWAITING_PROTOCOL_HEADER));
}

}
}
