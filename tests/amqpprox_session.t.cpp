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

/*                              Test Concerns
 *
 * This component is one of the heaviest in amqpprox, and critical for many of
 * the behaviours as it is the direct point of relation between the incoming
 * and outgoing sockets, and the dataflow amongst them. As such it has a number
 * of test concerns and quite a lot of apparatus required to test it in
 * 'isolation'.
 *
 * Tests:
 *
 * [x] End-to-end connection establishment: handshaking with both the 'client'
 *     and a simulated 'broker'
 * [x] TLS handshake failure with the newly connected 'client'
 * [x] Proxy protocol header is inserted before the AMQP header
 * [ ] Handshaking fails due to a bad message
 * [ ] Handshaking fails due to no broker to connect to
 * [ ] Handshaking succeeds after first broker connection fails and needs to be
 *     retried
 *
 */

#define SOCKET_TESTING 1

#include <amqpprox_backendset.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_connectionmanager.h>
#include <amqpprox_connectionselector.h>
#include <amqpprox_connectorutil.h>
#include <amqpprox_constants.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_hostnamemapper.h>
#include <amqpprox_methods_close.h>
#include <amqpprox_methods_closeok.h>
#include <amqpprox_methods_startok.h>
#include <amqpprox_methods_tuneok.h>
#include <amqpprox_reply.h>
#include <amqpprox_robinbackendselector.h>
#include <amqpprox_session.h>
#include <amqpprox_sessionstate.h>
#include <amqpprox_socketintercept.h>
#include <amqpprox_socketintercepttestadaptor.h>
#include <amqpprox_testsocketstate.h>

#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;
using Item              = TestSocketState::Item;
using Func              = TestSocketState::Func;
using Data              = TestSocketState::Data;
using Call              = TestSocketState::Call;
using HandshakeComplete = TestSocketState::HandshakeComplete;
using WriteComplete     = TestSocketState::WriteComplete;
using ConnectComplete   = TestSocketState::ConnectComplete;

struct SelectorMock : public ConnectionSelector {
    virtual ~SelectorMock() {}

    MOCK_METHOD2(acquireConnection,
                 int(std::shared_ptr<ConnectionManager> *,
                     const SessionState &));
};

struct HostnameMapperMock : public HostnameMapper {
    MOCK_METHOD2(prime,
                 void(boost::asio::io_service &,
                      std::initializer_list<boost::asio::ip::tcp::endpoint>));

    MOCK_CONST_METHOD1(mapToHostname,
                       std::string(const boost::asio::ip::tcp::endpoint &));
};

boost::asio::ip::tcp::endpoint makeEndpoint(const std::string &str,
                                            uint16_t           port)
{
    return boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address_v4::from_string(str), port);
}

class SessionTest : public ::testing::Test {
  protected:
    boost::asio::io_service             d_ioService;
    BufferPool                          d_pool;
    Backend                             d_backend1;
    Backend                             d_backend2;
    Backend                             d_backend3;
    std::shared_ptr<HostnameMapperMock> d_mapper;
    SelectorMock                        d_selector;
    RobinBackendSelector                d_robinSelector;
    std::shared_ptr<ConnectionManager>  d_cm;
    EventSource                         d_eventSource;
    TestSocketState                     d_clientState;
    TestSocketState                     d_serverState;
    SocketInterceptTestAdaptor          d_clientSocketAdaptor;
    SocketInterceptTestAdaptor          d_serverSocketAdaptor;
    SocketIntercept                     d_client;
    SocketIntercept                     d_server;
    int                                 d_step;

    SessionTest();

    void driveTo(int targetStep);

    template <typename METH>
    std::vector<uint8_t>
    encode(const METH &method, int type = 1, int channel = 0);

    std::vector<uint8_t> encodeHeartbeat();

    template <typename METH>
    std::optional<METH> decode(const std::vector<uint8_t> &buffer);

    methods::StartOk clientStartOk();
    methods::TuneOk  clientTuneOk();
    methods::Open    clientOpen();
    methods::Start   serverStart();
    methods::Tune    serverTune();
    methods::OpenOk  serverOpenOk();
    methods::CloseOk closeOk();
    methods::Close   close();

    void runStandardConnect(TestSocketState::State* clientBase);
};

template <typename TYPE>
std::vector<TYPE> filterVariant(const std::vector<Item> &items);

Data coalesce(std::initializer_list<Data> input);

void SessionTest::runStandardConnect(TestSocketState::State* clientBase)
{

    auto protocolHeader = std::vector<uint8_t>(
        Constants::protocolHeader(),
        Constants::protocolHeader() + Constants::protocolHeaderLength());

    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    d_serverState.pushItem(1, HandshakeComplete());

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    d_serverState.pushItem(2, Data(protocolHeader));
    d_serverState.expect(2, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(ConnectorUtil::synthesizedStart())));
    });

    //
    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    d_serverState.pushItem(3, Data(encode(clientStartOk())));
    d_serverState.expect(3, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(ConnectorUtil::synthesizedTune())));
    });

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    d_serverState.pushItem(4, Data(encode(clientTuneOk())));
    d_serverState.pushItem(4, Data(encode(clientOpen())));
    d_clientState.expect(4, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("async_connect"))));
    });

    // Client                       Proxy  <----TCP CONNECT---->  Broker
    d_clientState.pushItem(5, *clientBase);
    d_clientState.pushItem(5, ConnectComplete());
    d_clientState.expect(5, [](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 0);
    });

    // Client                       Proxy  <-----HANDSHAKE----->  Broker
    // Client                       Proxy  -----AMQP Header---->  Broker
    d_clientState.pushItem(6, HandshakeComplete());
    d_clientState.expect(6, [this, protocolHeader](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(protocolHeader));
    });

    // Client                       Proxy  <-------Start--------  Broker
    // Client                       Proxy  --------StartOk----->  Broker
    d_clientState.pushItem(7, Data(encode(serverStart())));
    d_clientState.expect(7, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);

        auto startOk = clientStartOk();
        ConnectorUtil::injectClientLocation(&startOk, "host1", 2345);

        EXPECT_EQ(data[0], Data(encode(startOk)));
    });

    // Client                       Proxy  <-------Tune--------  Broker
    // Client                       Proxy  --------TuneOk----->  Broker
    // Client                       Proxy  --------Open------->  Broker
    d_clientState.pushItem(8, Data(encode(serverTune())));
    d_clientState.expect(8, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0],
                  coalesce({encode(clientTuneOk()), encode(clientOpen())}));
    });

    // Client  <-----OpenOk-------  Proxy  <-------OpenOk------  Broker
    d_clientState.pushItem(9, Data(encode(serverOpenOk())));
    d_serverState.expect(9, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(serverOpenOk())));
    });

    // Client  <-----Heartbeat----  Proxy  <-------Heartbeat---  Broker
    d_clientState.pushItem(10, Data(encodeHeartbeat()));
    d_serverState.expect(10, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encodeHeartbeat()));
    });

    // Client  ------Heartbeat--->  Proxy  --------Heartbeat-->  Broker
    d_serverState.pushItem(11, Data(encodeHeartbeat()));
    d_clientState.expect(11, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encodeHeartbeat()));
    });

    // session->disconnect(false) called (Set up later after session object is
    // created).
    // Client  <-----Close--------  Proxy                        Broker
    d_serverState.expect(12, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(close())));
    });

    // Client  ------CloseOk----->  Proxy                        Broker
    // Client                       Proxy  --------Close------>  Broker
    d_serverState.pushItem(13, Data(encode(closeOk())));
    d_clientState.expect(13, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        std::cerr << "Expected: " << close() << std::endl;
        auto decoded = decode<methods::Close>(data[0].d_value);
        if (decoded) {
            std::cerr << "Got: " << *decoded << std::endl;
        }
        else {
            std::cerr << "couldn't decode" << std::endl;
        }
        EXPECT_EQ(data[0], Data(encode(close())));
    });

    // Client                       Proxy  <-------CloseOk-----  Broker
    // Proxy snaps all TCP connections at this point.
    // NB: In reality the client/broker may beat us by snapping their end
    // before we get to close ourselves.
    d_clientState.pushItem(14, Data(encode(closeOk())));
    d_clientState.expect(14, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_serverState.expect(14, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });

    // After closing sockets any outstanding handlers will get aborted
    d_serverState.pushItem(15, Data(boost::asio::error::operation_aborted));
    d_clientState.pushItem(16, Data(boost::asio::error::operation_aborted));

}

TEST_F(SessionTest, Connection_Then_Ping_Then_Disconnect)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm), Return(0)));

    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 1234)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 32000)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("3.4.5.6", 5672)))
        .WillRepeatedly(Return(std::string("host2")));

    TestSocketState::State base;
    base.d_local  = makeEndpoint("1.2.3.4", 1234);
    base.d_remote = makeEndpoint("2.3.4.5", 2345);
    base.d_secure = false;

    TestSocketState::State clientBase;
    clientBase.d_local  = makeEndpoint("1.2.3.4", 32000);
    clientBase.d_remote = makeEndpoint("3.4.5.6", 5672);
    clientBase.d_secure = false;

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnect(&clientBase);

    MaybeSecureSocketAdaptor clientSocket(d_ioService, d_client, false);
    MaybeSecureSocketAdaptor serverSocket(d_ioService, d_server, false);
    auto                     session = std::make_shared<Session>(d_ioService,
                                             std::move(serverSocket),
                                             std::move(clientSocket),
                                             &d_selector,
                                             &d_eventSource,
                                             &d_pool,
                                             d_mapper);

    session->start();

    // Graceful disconnect after the heartbeats
    d_serverState.pushItem(12,
                           Func([&session] { session->disconnect(false); }));

    // Lastly, check it's elligible to be deleted
    d_serverState.pushItem(
        17, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_CLEANLY);
        }));

    // Run the tests through to completion
    driveTo(17);
}

TEST_F(SessionTest, New_Client_Handshake_Failure)
{
    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("0.0.0.0", 0)))
        .WillRepeatedly(Return(std::string("host0")));

    TestSocketState::State base;
    base.d_local  = makeEndpoint("1.2.3.4", 1234);
    base.d_remote = makeEndpoint("2.3.4.5", 2345);
    base.d_secure = false;

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    d_serverState.pushItem(
        1, HandshakeComplete(boost::asio::error::access_denied));
    d_serverState.expect(1, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_clientState.expect(1, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });

    MaybeSecureSocketAdaptor clientSocket(d_ioService, d_client, false);
    MaybeSecureSocketAdaptor serverSocket(d_ioService, d_server, false);
    auto                     session = std::make_shared<Session>(d_ioService,
                                             std::move(serverSocket),
                                             std::move(clientSocket),
                                             &d_selector,
                                             &d_eventSource,
                                             &d_pool,
                                             d_mapper);

    session->start();

    // Run the tests through to completion
    driveTo(1);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, Connection_To_Proxy_Protocol)
{
    Backend backendPP("backend1", "dc1", "localhost", "127.0.0.1", 5672, true);
    std::vector<BackendSet::Partition> partitions;
    partitions.push_back(BackendSet::Partition{&backendPP});

    auto cm = std::make_shared<ConnectionManager>(
        std::make_shared<BackendSet>(partitions), &d_robinSelector);
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(cm), Return(0)));

    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 1234)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 32000)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("3.4.5.6", 5672)))
        .WillRepeatedly(Return(std::string("host2")));

    auto protocolHeader = std::vector<uint8_t>(
        Constants::protocolHeader(),
        Constants::protocolHeader() + Constants::protocolHeaderLength());

    TestSocketState::State base;
    base.d_local  = makeEndpoint("1.2.3.4", 1234);
    base.d_remote = makeEndpoint("2.3.4.5", 2345);
    base.d_secure = false;

    TestSocketState::State clientBase;
    clientBase.d_local  = makeEndpoint("1.2.3.4", 32000);
    clientBase.d_remote = makeEndpoint("3.4.5.6", 5672);
    clientBase.d_secure = false;

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnect(&clientBase);

    MaybeSecureSocketAdaptor clientSocket(d_ioService, d_client, false);
    MaybeSecureSocketAdaptor serverSocket(d_ioService, d_server, false);
    auto                     session = std::make_shared<Session>(d_ioService,
                                             std::move(serverSocket),
                                             std::move(clientSocket),
                                             &d_selector,
                                             &d_eventSource,
                                             &d_pool,
                                             d_mapper);

    session->start();

    // Override the expectation for proxy protocol
    d_clientState.expect(5, [&session, &backendPP](const auto &items) {
        auto data = filterVariant<Data>(items);

        ASSERT_EQ(data.size(), 1);
        auto generatedPPHeader = session->getProxyProtocolHeader(&backendPP);
        std::vector<uint8_t> generatedPPHeaderVec(generatedPPHeader.data(),
                             generatedPPHeader.data() +
                                 generatedPPHeader.length());
        ASSERT_EQ(data[0], Data(generatedPPHeaderVec));
    });

    // Graceful disconnect after the heartbeats
    d_serverState.pushItem(12,
                           Func([&session] { session->disconnect(false); }));

    // Lastly, check it's elligible to be deleted
    d_serverState.pushItem(
        17, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_CLEANLY);
        }));

    // Run the tests through to completion
    driveTo(17);
}
///////////////////////////////////////////////////////////////////////////////
//
//     Test Apparatus
//
///////////////////////////////////////////////////////////////////////////////

template <typename TYPE>
std::vector<TYPE> filterVariant(const std::vector<Item> &items)
{
    std::vector<TYPE> out;
    for (const auto &item : items) {
        if (auto inc = std::get_if<TYPE>(&item)) {
            out.push_back(*inc);
        }
    }
    return out;
}

Data coalesce(std::initializer_list<Data> input)
{
    std::vector<uint8_t>      buffer;
    boost::system::error_code ec;
    for (const auto &i : input) {
        if (i.d_ec) {
            ec = i.d_ec;
        }

        auto &buf = i.d_value;
        buffer.insert(buffer.end(), buf.begin(), buf.end());
    }
    return Data(buffer, ec);
}

SessionTest::SessionTest()
: d_ioService()
, d_pool({32,
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
, d_backend1("backend1", "dc1", "localhost", "127.0.0.1", 5672)
, d_backend2("backend2", "dc2", "localhost", "127.0.0.1", 5673)
, d_backend3("backend3", "dc3", "localhost", "127.0.0.1", 5674)
, d_mapper(new HostnameMapperMock)
, d_selector()
, d_robinSelector()
, d_cm()
, d_eventSource()
, d_clientState()
, d_serverState()
, d_clientSocketAdaptor(d_clientState)
, d_serverSocketAdaptor(d_serverState)
, d_client(d_clientSocketAdaptor)
, d_server(d_serverSocketAdaptor)
, d_step(0)
{
    std::vector<BackendSet::Partition> partitions;
    partitions.push_back(
        BackendSet::Partition{&d_backend1, &d_backend2, &d_backend3});

    d_cm = std::make_shared<ConnectionManager>(
        std::make_shared<BackendSet>(partitions), &d_robinSelector);
}

void SessionTest::driveTo(int targetStep)
{
    for (; d_step <= targetStep && !HasFatalFailure(); ++d_step) {
        std::cerr << "=========== Driving Step " << d_step << std::endl;
        d_clientState.setCurrentStep(d_step);
        d_serverState.setCurrentStep(d_step);

        bool drive = true;
        while (drive) {
            bool clientHadWork = d_clientState.drive();
            bool serverHadWork = d_serverState.drive();
            bool ioloopHadWork = d_ioService.run() > 0;
            d_ioService.restart();
            drive = clientHadWork || serverHadWork || ioloopHadWork;
        }

        d_clientState.validate();
        d_serverState.validate();
    }
}

template <typename METH>
std::vector<uint8_t>
SessionTest::encode(const METH &method, int type, int channel)
{
    BufferHandle tempBuffer;
    BufferHandle outputBuffer;
    d_pool.acquireBuffer(&tempBuffer, Frame::getMaxFrameSize());

    Buffer buildResponse(tempBuffer.data(), tempBuffer.size());
    if (Method::encode(buildResponse, method)) {
        Frame f;
        f.type                   = 1;
        f.channel                = 0;
        f.payload                = buildResponse.originalPtr();
        f.length                 = buildResponse.offset();
        std::size_t returnedData = 0;
        std::size_t newLength    = f.length + Frame::frameOverhead();

        d_pool.acquireBuffer(&outputBuffer, newLength);

        auto startPtr = static_cast<char *>(outputBuffer.data());
        if (Frame::encode(startPtr, &returnedData, f)) {
            return std::vector<uint8_t>(startPtr, startPtr + returnedData);
        }
        else {
            throw std::runtime_error("Cannot encode test frame");
        }
    }
    else {
        throw std::runtime_error("Cannot encode test method");
    }
}

std::vector<uint8_t> SessionTest::encodeHeartbeat()
{
    BufferHandle outputBuffer;
    Frame        f;
    f.type                   = 8;
    f.channel                = 0;
    f.payload                = nullptr;
    f.length                 = 0;
    std::size_t newLength    = Frame::frameOverhead();
    std::size_t returnedData = 0;

    d_pool.acquireBuffer(&outputBuffer, newLength);

    auto startPtr = static_cast<char *>(outputBuffer.data());
    if (Frame::encode(startPtr, &returnedData, f)) {
        return std::vector<uint8_t>(startPtr, startPtr + returnedData);
    }
    else {
        throw std::runtime_error("Cannot encode test frame");
    }
}

template <typename METH>
std::optional<METH> SessionTest::decode(const std::vector<uint8_t> &buffer)
{
    Frame       frame;
    const void *endOfFrame = nullptr;
    std::size_t remaining  = 0;
    if (Frame::decode(
            &frame, &endOfFrame, &remaining, buffer.data(), buffer.size())) {
        Method method;
        if (Method::decode(&method, frame.payload, frame.length)) {
            Buffer methodPayload(method.payload, method.length);
            if (METH::methodType() == method.methodType) {
                METH result;
                if (METH::decode(&result, methodPayload)) {
                    return std::optional<METH>(result);
                }
                std::cerr << "Couldn't decode the method" << std::endl;
            }
            std::cerr << "Wrong method type: " << method.methodType
                      << std::endl;
        }
        std::cerr << "Couldn't decode the method header" << std::endl;
    }
    std::cerr << "Couldn't decode the frame header" << std::endl;
    return std::optional<METH>();
}

methods::StartOk SessionTest::clientStartOk()
{
    return methods::StartOk();
}

methods::TuneOk SessionTest::clientTuneOk()
{
    return methods::TuneOk();
}

methods::Open SessionTest::clientOpen()
{
    return methods::Open();
}

methods::Start SessionTest::serverStart()
{
    return methods::Start();
}

methods::Tune SessionTest::serverTune()
{
    return methods::Tune();
}

methods::OpenOk SessionTest::serverOpenOk()
{
    return methods::OpenOk();
}

methods::CloseOk SessionTest::closeOk()
{
    return methods::CloseOk();
}

methods::Close SessionTest::close()
{
    auto result = methods::Close();
    result.setReply(Reply::OK::CODE, Reply::OK::TEXT);
    return result;
}
