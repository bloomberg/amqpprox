/*
** Copyright 2022 Bloomberg Finance L.P.
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

#include <amqpprox_maybesecuresocketadaptor.h>

#include <boost/asio/buffer.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>

using namespace Bloomberg;
using namespace amqpprox;

using namespace ::testing;

using DummyIoContext  = int;
using DummyTlsContext = int;

class SocketInterface {
  public:
    virtual void
    async_read_some(boost::asio::null_buffers,
                    std::function<void(const boost::system::error_code &error,
                                       size_t numBytes)> handler) = 0;

    virtual void
    async_read_some(boost::asio::mutable_buffers_1,
                    std::function<void(const boost::system::error_code &error,
                                       size_t numBytes)> handler) = 0;

    virtual size_t read_some(const boost::asio::mutable_buffers_1 &buffer,
                             boost::system::error_code            &error) = 0;

    virtual ~SocketInterface(){};
};

class MockSocket : public SocketInterface {
  public:
    static MockSocket *instance;
    MockSocket(DummyIoContext &) { instance = this; };
    MockSocket(DummyIoContext &, DummyTlsContext &) { instance = this; };
    typedef int executor_type;

    MOCK_METHOD2(
        async_read_some,
        void(boost::asio::null_buffers,
             std::function<void(const boost::system::error_code &, size_t)>));

    MOCK_METHOD2(
        async_read_some,
        void(boost::asio::mutable_buffers_1,
             std::function<void(const boost::system::error_code &, size_t)>));

    MOCK_METHOD2(read_some,
                 size_t(const boost::asio::mutable_buffers_1 &,
                        boost::system::error_code &));

    MockSocket &next_layer() { return *this; }
};

class TimerInterface {
  public:
    virtual ~TimerInterface(){};
    virtual void cancel() = 0;

    virtual size_t
    expires_at(const std::chrono::steady_clock::time_point &expiry_time) = 0;

    virtual std::chrono::steady_clock::time_point expiry() = 0;

    virtual size_t
    expires_after(const std::chrono::steady_clock::duration &expiry_time) = 0;

    virtual void
    async_wait(std::function<void(const boost::system::error_code &error)>
                   handler) = 0;
};

class MockTimer : public TimerInterface {
  public:
    static MockTimer *instance;
    MockTimer(int &) { instance = this; };

    MOCK_METHOD0(cancel, void());
    MOCK_METHOD1(expires_at,
                 size_t(const std::chrono::steady_clock::time_point &));
    MOCK_METHOD0(expiry, std::chrono::steady_clock::time_point());
    MOCK_METHOD1(expires_after,
                 size_t(const std::chrono::steady_clock::duration &));
    MOCK_METHOD1(
        async_wait,
        void(std::function<void(const boost::system::error_code &error)>));
};

MockTimer  *MockTimer::instance  = nullptr;
MockSocket *MockSocket::instance = nullptr;

TEST(MaybeSecureSocketAdaptor, alive)
{
    DummyIoContext  ioContext  = 5;
    DummyTlsContext tlsContext = 5;

    MaybeSecureSocketAdaptor<MockSocket,
                             MockTimer,
                             DummyIoContext,
                             DummyTlsContext>
        socket(ioContext, tlsContext, false);
}

TEST(MaybeSecureSocketAdaptorDataRateLimit, LimitEventuallyHit)
{
    DummyIoContext  ioContext  = 5;
    DummyTlsContext tlsContext = 5;
    MaybeSecureSocketAdaptor<MockSocket,
                             MockTimer,
                             DummyIoContext,
                             DummyTlsContext>
        socket(ioContext, tlsContext, false);

    // This test requires a few timer methods. At this point we don't
    // explicitly test these, but the fact they are called is still worth
    // asserting
    EXPECT_CALL(*MockTimer::instance, cancel()).WillRepeatedly(Return());
    EXPECT_CALL(*MockTimer::instance, expires_at(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*MockTimer::instance, expires_after(_))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*MockTimer::instance, expiry())
        .WillRepeatedly(Return(std::chrono::steady_clock::time_point()));
    EXPECT_CALL(*MockTimer::instance, async_wait(_)).WillRepeatedly(Return());

    socket.setReadRateLimit(50);

    std::function<void(const boost::system::error_code &, size_t)>
        asyncReadHandler;

    std::vector<uint8_t> data;
    data.resize(128);
    boost::asio::mutable_buffers_1 buffer(&data, 128);

    {
        // this will be invoked because we definitely haven't hit the limit yet
        EXPECT_CALL(*MockSocket::instance,
                    async_read_some(An<boost::asio::null_buffers>(), _))
            .WillOnce(SaveArg<1>(&asyncReadHandler));

        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code &error, size_t numBytes) {

            });

        asyncReadHandler(boost::system::error_code(), 55);

        EXPECT_CALL(*MockSocket::instance, read_some(_, _))
            .WillOnce(Return(55));

        boost::system::error_code ec;
        EXPECT_EQ(55, socket.read_some(buffer, ec));
    }

    {
        // MockSocket.async_read_some will be invoked here because we do not
        // trigger any data limiting on the first limit breach. This call will
        // start the timers etc preparing to rate limit for real the next time
        // this limit is hit
        EXPECT_CALL(*MockSocket::instance,
                    async_read_some(An<boost::asio::null_buffers>(), _))
            .WillOnce(SaveArg<1>(&asyncReadHandler));
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code &error, size_t numBytes) {

            });

        EXPECT_CALL(*MockSocket::instance, read_some(_, _))
            .WillOnce(Return(55));
        boost::system::error_code ec;
        EXPECT_EQ(55, socket.read_some(buffer, ec));
    }

    {
        // We are expecting that MockSocket.async_read_some will not be invoked
        // again because we've read_some'd up to our data rate limit without
        // invoking the timer handler
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code &error, size_t numBytes) {

            });
    }
}

TEST(MaybeSecureSocketAdaptorDataRateLimit, TimerHandlerLifetimes)
{
    DummyIoContext  ioContext  = 5;
    DummyTlsContext tlsContext = 5;

    std::function<void(const boost::system::error_code &error)> timerHandler;

    {
        MaybeSecureSocketAdaptor<MockSocket,
                                 MockTimer,
                                 DummyIoContext,
                                 DummyTlsContext>
            socket(ioContext, tlsContext, false);

        // This test requires a few timer methods. At this point we don't
        // explicitly test these, but the fact they are called is still worth
        // asserting
        EXPECT_CALL(*MockTimer::instance, expires_at(_))
            .WillRepeatedly(Return(0));
        EXPECT_CALL(*MockTimer::instance, expires_after(_))
            .WillRepeatedly(Return(0));
        EXPECT_CALL(*MockTimer::instance, expiry())
            .WillRepeatedly(Return(std::chrono::steady_clock::time_point()));
        EXPECT_CALL(*MockTimer::instance, async_wait(_))
            .WillRepeatedly(SaveArg<0>(&timerHandler));
        EXPECT_CALL(*MockTimer::instance, cancel()).WillRepeatedly(Return());

        socket.setReadRateLimit(50);

        std::function<void(const boost::system::error_code &, size_t)>
            asyncReadHandler;

        std::vector<uint8_t> data;
        data.resize(128);
        boost::asio::mutable_buffers_1 buffer(&data, 128);

        {
            // this will be invoked because we definitely haven't hit the limit
            // yet
            EXPECT_CALL(*MockSocket::instance,
                        async_read_some(An<boost::asio::null_buffers>(), _))
                .WillOnce(SaveArg<1>(&asyncReadHandler));

            socket.async_read_some(
                boost::asio::null_buffers(),
                [&](const boost::system::error_code &error, size_t numBytes) {

                });

            asyncReadHandler(boost::system::error_code(), 55);

            EXPECT_CALL(*MockSocket::instance, read_some(_, _))
                .WillOnce(Return(55));

            boost::system::error_code ec;
            EXPECT_EQ(55, socket.read_some(buffer, ec));
        }

        {
            // MockSocket.async_read_some will be invoked here because we do
            // not trigger any data limiting on the first limit breach. This
            // call will start the timers etc preparing to rate limit for real
            // the next time this limit is hit
            EXPECT_CALL(*MockSocket::instance,
                        async_read_some(An<boost::asio::null_buffers>(), _))
                .WillOnce(SaveArg<1>(&asyncReadHandler));
            socket.async_read_some(
                boost::asio::null_buffers(),
                [&](const boost::system::error_code &error, size_t numBytes) {

                });

            EXPECT_CALL(*MockSocket::instance, read_some(_, _))
                .WillOnce(Return(55));
            boost::system::error_code ec;
            EXPECT_EQ(55, socket.read_some(buffer, ec));
        }

        {
            // We are expecting that MockSocket.async_read_some will not be
            // invoked again because we've read_some'd up to our data rate
            // limit without invoking the timer handler
            socket.async_read_some(
                boost::asio::null_buffers(),
                [&](const boost::system::error_code &error, size_t numBytes) {

                });
        }
    }
    // MaybeSecureSocketAdaptor destructed above

    // This shouldn't crash / report an error in valgrind
    timerHandler(boost::asio::error::operation_aborted);
}
