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
 * [X] Handshaking fails due to no broker to connect to
 * [X] Handshaking succeeds after first broker connection fails and needs to be
 *     retried
 * [ ] Handshaking fails due to a bad message / garbage content / fuzzing
 * [ ] Simulate failures of less common conditions, such as error codes from
 *     shutdown/close/setting socket options
 */

#define SOCKET_TESTING 1

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_backendset.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_connectionmanager.h>
#include <amqpprox_connectionselectorinterface.h>
#include <amqpprox_connectorutil.h>
#include <amqpprox_constants.h>
#include <amqpprox_dataratelimitmanager.h>
#include <amqpprox_defaultauthintercept.h>
#include <amqpprox_dnsresolver.h>
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
#include <authrequest.pb.h>
#include <authresponse.pb.h>

#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
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

const char LOCAL_HOSTNAME[] = "amqpprox-host";

// To fix compilation error (gtest ver >=1.12.1) in EXPECT_THAT macro when
// validating TestSocketState::Item. Since TestSocketState::Item is a variant,
// googletest requires operator<< / PrintTo to be defined for every member of
// the variant.
void PrintTo(const TestSocketState::Func &, std::ostream *stream)
{
    *stream << "invoked PrintTo for object of type 'const "
               "TestSocketState::Func'\n";
}

struct SelectorMock : public ConnectionSelectorInterface {
    virtual ~SelectorMock() {}

    MOCK_METHOD2(
        acquireConnection,
        SessionState::ConnectionStatus(std::shared_ptr<ConnectionManager> *,
                                       const SessionState &));
};

struct HostnameMapperMock : public HostnameMapper {
    MOCK_METHOD2(prime,
                 void(boost::asio::io_context &,
                      std::initializer_list<boost::asio::ip::tcp::endpoint>));

    MOCK_CONST_METHOD1(mapToHostname,
                       std::string(const boost::asio::ip::tcp::endpoint &));
};

struct AuthInterceptInterfaceMock : public AuthInterceptInterface {
    virtual ~AuthInterceptInterfaceMock() {}

    AuthInterceptInterfaceMock(boost::asio::io_context &ioContext)
    : AuthInterceptInterface(ioContext)
    {
    }

    MOCK_CONST_METHOD1(print, void(std::ostream &));
    MOCK_METHOD2(authenticate,
                 void(const authproto::AuthRequest,
                      const AuthInterceptInterface::ReceiveResponseCb &));
};

boost::asio::ip::tcp::endpoint makeEndpoint(const std::string &str,
                                            uint16_t           port)
{
    return boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address_v4::from_string(str), port);
}

class SessionTest : public ::testing::Test {
  protected:
    boost::asio::io_context                     d_ioContext;
    BufferPool                                  d_pool;
    Backend                                     d_backend1;
    Backend                                     d_backend2;
    Backend                                     d_backend3;
    std::shared_ptr<HostnameMapperMock>         d_mapper;
    DNSResolver                                 d_dnsResolver;
    SelectorMock                                d_selector;
    RobinBackendSelector                        d_robinSelector;
    std::shared_ptr<ConnectionManager>          d_cm;
    EventSource                                 d_eventSource;
    TestSocketState                             d_clientState;
    TestSocketState                             d_serverState;
    SocketInterceptTestAdaptor                  d_clientSocketAdaptor;
    SocketInterceptTestAdaptor                  d_serverSocketAdaptor;
    SocketIntercept                             d_client;
    SocketIntercept                             d_server;
    std::vector<uint8_t>                        d_protocolHeader;
    int                                         d_step;
    std::shared_ptr<AuthInterceptInterface>     d_authIntercept;
    std::shared_ptr<AuthInterceptInterfaceMock> d_mockAuthIntercept;
    DataRateLimitManager                        d_limitManager;

    SessionTest();

    std::shared_ptr<Session> makeSession(
        const std::shared_ptr<MaybeSecureSocketAdaptor<>> &clientSocket,
        const std::shared_ptr<MaybeSecureSocketAdaptor<>> &serverSocket,
        std::shared_ptr<AuthInterceptInterface>            authIntercept = 0);

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

    void runConnectToClientOpen(TestSocketState::State *clientBase);
    void runBrokerHandshake(TestSocketState::State *clientBase, int step);
    void runStandardConnect(TestSocketState::State *clientBase);
    void runStandardConnectWithDisconnect(TestSocketState::State *clientBase);

    void testSetupServerHandshake(int idx);
    void testSetupClientSendsProtocolHeader(int idx);
    void testSetupClientStartOk(
        int                     idx,
        const methods::StartOk &overriddenStartOk = methods::StartOk());
    void testSetupClientOpen(int idx);
    void testSetupClientOpenWithProxyClose(
        int                                    idx,
        const std::shared_ptr<methods::Close> &closeMethodPtr = nullptr);
    void testSetupClientOpenWithoutTune(int idx);
    void testSetupProxyConnect(int idx, TestSocketState::State *clientBase);
    void testSetupProxySendsProtocolHeader(int idx);
    void testSetupProxySendsStartOk(
        int                     idx,
        const std::string      &injectedClientHost,
        int                     injectedClientPort,
        std::string_view        injectedProxyHost,
        int                     injectedProxyInboundPort,
        int                     injectedProxyOutbountPort,
        const methods::StartOk &overriddenStartOk = methods::StartOk());
    void testSetupProxyOpen(int idx);
    void testSetupProxyOutOfOrderOpen(int idx);
    void testSetupProxyForwardsBrokerClose(int idx);
    void testSetupProxyPassOpenOkThrough(int idx);
    void testSetupBrokerSendsHeartbeat(int idx);
    void testSetupClientSendsHeartbeat(int idx);
    void testSetupProxySendsCloseToClient(int idx);
    void testSetupClientSendsCloseOk(int idx);
    void testSetupBrokerRespondsCloseOk(int idx);
    void testSetupHandlersCleanedUp(int idx);

    void testSetupHostnameMapperForServerClientBase(
        TestSocketState::State &serverState,
        TestSocketState::State &clientState);
};

std::shared_ptr<Session> SessionTest::makeSession(
    const std::shared_ptr<MaybeSecureSocketAdaptor<>> &clientSocket,
    const std::shared_ptr<MaybeSecureSocketAdaptor<>> &serverSocket,
    std::shared_ptr<AuthInterceptInterface>            authIntercept)
{
    if (!authIntercept) {
        authIntercept = d_authIntercept;
    }

    return std::make_shared<Session>(d_ioContext,
                                     serverSocket,
                                     clientSocket,
                                     &d_selector,
                                     &d_eventSource,
                                     &d_pool,
                                     &d_dnsResolver,
                                     d_mapper,
                                     LOCAL_HOSTNAME,
                                     authIntercept,
                                     false,
                                     &d_limitManager);
}

template <typename TYPE>
std::vector<TYPE> filterVariant(const std::vector<Item> &items);

Data coalesce(std::initializer_list<Data> input);

void SessionTest::testSetupHostnameMapperForServerClientBase(
    TestSocketState::State &base,
    TestSocketState::State &clientBase)
{
    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 1234)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 32000)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("3.4.5.6", 5672)))
        .WillRepeatedly(Return(std::string("host2")));

    base.d_local  = makeEndpoint("1.2.3.4", 1234);
    base.d_remote = makeEndpoint("2.3.4.5", 2345);
    base.d_secure = false;

    clientBase.d_local  = makeEndpoint("1.2.3.4", 32000);
    clientBase.d_remote = makeEndpoint("3.4.5.6", 5672);
    clientBase.d_secure = false;
}

void SessionTest::testSetupServerHandshake(int idx)
{
    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    d_serverState.pushItem(idx, HandshakeComplete());
}

void SessionTest::testSetupClientSendsProtocolHeader(int idx)
{
    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    d_serverState.pushItem(idx, Data(d_protocolHeader));
    d_serverState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(ConnectorUtil::synthesizedStart())));
    });
}

void SessionTest::testSetupClientStartOk(
    int                     idx,
    const methods::StartOk &overriddenStartOk)
{
    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    d_serverState.pushItem(idx, Data(encode(overriddenStartOk)));
    d_serverState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(ConnectorUtil::synthesizedTune())));
    });
}

void SessionTest::testSetupClientOpen(int idx)
{
    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    d_serverState.pushItem(idx, Data(encode(clientTuneOk())));
    d_serverState.pushItem(idx, Data(encode(clientOpen())));
}

void SessionTest::testSetupClientOpenWithProxyClose(
    int                                    idx,
    const std::shared_ptr<methods::Close> &closeMethodPtr)
{
    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    // Client  <-----Close--------  Proxy                         Broker
    d_serverState.pushItem(idx, Data(encode(clientTuneOk())));
    d_serverState.pushItem(idx, Data(encode(clientOpen())));

    d_serverState.expect(idx, [this, closeMethodPtr](const auto &items) {
        if (closeMethodPtr) {
            auto data = filterVariant<Data>(items);
            ASSERT_EQ(data.size(), 1);
            EXPECT_EQ(data[0], Data(encode(*closeMethodPtr)));
        }
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
}

void SessionTest::testSetupClientOpenWithoutTune(int idx)
{
    // Client  ------Open-------->  Proxy                         Broker
    d_serverState.pushItem(idx, Data(encode(clientOpen())));
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
    });
}

void SessionTest::testSetupProxyConnect(int                     idx,
                                        TestSocketState::State *clientBase)
{
    // Client                       Proxy  <----TCP CONNECT---->  Broker
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("async_connect"))));
    });
    d_clientState.pushItem(idx, *clientBase);
    d_clientState.pushItem(idx, ConnectComplete());
    d_clientState.expect(idx, [](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 0);
    });
}

void SessionTest::testSetupProxySendsProtocolHeader(int idx)
{
    // Client                       Proxy  <-----HANDSHAKE----->  Broker
    // Client                       Proxy  -----AMQP Header---->  Broker
    d_clientState.pushItem(idx, HandshakeComplete());
    d_clientState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(d_protocolHeader));
    });
}

void SessionTest::testSetupProxySendsStartOk(
    int                     idx,
    const std::string      &injectedClientHost,
    int                     injectedClientPort,
    std::string_view        injectedProxyHost,
    int                     injectedProxyInboundPort,
    int                     injectedProxyOutboundPort,
    const methods::StartOk &overriddenStartOk)
{
    // Client                       Proxy  <-------Start--------  Broker
    // Client                       Proxy  --------StartOk----->  Broker
    d_clientState.pushItem(idx, Data(encode(serverStart())));
    d_clientState.expect(idx,
                         [this,
                          injectedClientHost,
                          injectedClientPort,
                          injectedProxyHost,
                          injectedProxyInboundPort,
                          injectedProxyOutboundPort,
                          overriddenStartOk](const auto &items) {
                             auto data = filterVariant<Data>(items);
                             ASSERT_EQ(data.size(), 1);

                             auto startOk = overriddenStartOk;
                             ConnectorUtil::injectProxyClientIdent(
                                 &startOk,
                                 injectedClientHost,
                                 injectedClientPort,
                                 injectedProxyHost,
                                 injectedProxyInboundPort,
                                 injectedProxyOutboundPort,
                                 false);

                             EXPECT_EQ(data[0], Data(encode(startOk)));
                         });
}

void SessionTest::testSetupProxyOutOfOrderOpen(int idx)
{
    // Client                       Proxy  <--------OpenOk------ Broker
    d_clientState.pushItem(idx, Data(encode(serverOpenOk())));
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
    });
    d_serverState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
    });
}

void SessionTest::testSetupProxyForwardsBrokerClose(int idx)
{
    // Client  <--------Close-----  Proxy  <--------Close------  Broker
    methods::Close receivedClose;
    receivedClose.setReply(123, "Broker is closing the connection");
    d_clientState.pushItem(idx, Data(encode(receivedClose)));

    d_serverState.expect(idx, [this, receivedClose](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(receivedClose)));
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
}

void SessionTest::testSetupProxyOpen(int idx)
{
    // Client                       Proxy  <-------Tune--------  Broker
    // Client                       Proxy  --------TuneOk----->  Broker
    // Client                       Proxy  --------Open------->  Broker
    d_clientState.pushItem(idx, Data(encode(serverTune())));
    d_clientState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0],
                  coalesce({encode(clientTuneOk()), encode(clientOpen())}));
    });
}

void SessionTest::testSetupProxyPassOpenOkThrough(int idx)
{
    // Client  <-----OpenOk-------  Proxy  <-------OpenOk------  Broker
    d_clientState.pushItem(idx, Data(encode(serverOpenOk())));
    d_serverState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(serverOpenOk())));
    });
}

void SessionTest::testSetupBrokerSendsHeartbeat(int idx)
{
    // Client  <-----Heartbeat----  Proxy  <-------Heartbeat---  Broker
    d_clientState.pushItem(idx, Data(encodeHeartbeat()));
    d_serverState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encodeHeartbeat()));
    });
}

void SessionTest::testSetupClientSendsHeartbeat(int idx)
{
    // Client  ------Heartbeat--->  Proxy  --------Heartbeat-->  Broker
    d_serverState.pushItem(idx, Data(encodeHeartbeat()));
    d_clientState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encodeHeartbeat()));
    });
}

void SessionTest::testSetupProxySendsCloseToClient(int idx)
{
    // session->disconnect(false) called (Set up later after session object is
    // created).
    //
    // Client  <-----Close--------  Proxy                        Broker
    d_serverState.expect(idx, [this](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], Data(encode(close())));
    });
}

void SessionTest::testSetupClientSendsCloseOk(int idx)
{
    // Client  ------CloseOk----->  Proxy                        Broker
    // Client                       Proxy  --------Close------>  Broker
    d_serverState.pushItem(idx, Data(encode(closeOk())));
    d_clientState.expect(idx, [this](const auto &items) {
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
}

void SessionTest::testSetupBrokerRespondsCloseOk(int idx)
{
    // Client                       Proxy  <-------CloseOk-----  Broker
    // Proxy snaps all TCP connections at this point.
    // NB: In reality the client/broker may beat us by snapping their end
    // before we get to close ourselves.
    d_clientState.pushItem(idx, Data(encode(closeOk())));
    d_clientState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_serverState.expect(idx, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
}

void SessionTest::testSetupHandlersCleanedUp(int idx)
{
    d_serverState.pushItem(idx, Data(boost::asio::error::operation_aborted));
    d_clientState.pushItem(idx, Data(boost::asio::error::operation_aborted));
}

void SessionTest::runConnectToClientOpen(TestSocketState::State *clientBase)
{
    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    testSetupClientOpen(4);
}

void SessionTest::runBrokerHandshake(TestSocketState::State *clientBase,
                                     int                     step)
{
    // Client                       Proxy  <----TCP CONNECT---->  Broker
    testSetupProxyConnect(step + 0, clientBase);

    // Client                       Proxy  <-----HANDSHAKE----->  Broker
    // Client                       Proxy  -----AMQP Header---->  Broker
    testSetupProxySendsProtocolHeader(step + 1);

    // Client                       Proxy  <-------Start--------  Broker
    // Client                       Proxy  --------StartOk----->  Broker
    testSetupProxySendsStartOk(
        step + 2, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);

    // Client                       Proxy  <-------Tune--------  Broker
    // Client                       Proxy  --------TuneOk----->  Broker
    // Client                       Proxy  --------Open------->  Broker
    testSetupProxyOpen(step + 3);
}

void SessionTest::runStandardConnect(TestSocketState::State *clientBase)
{
    // Run the whole handshake apart from OpenOk
    runConnectToClientOpen(clientBase);

    runBrokerHandshake(clientBase, 5);

    // Client  <-----OpenOk-------  Proxy  <-------OpenOk------  Broker
    testSetupProxyPassOpenOkThrough(9);
}

void SessionTest::runStandardConnectWithDisconnect(
    TestSocketState::State *clientBase)
{
    runStandardConnect(clientBase);

    // Client  <-----Heartbeat----  Proxy  <-------Heartbeat---  Broker
    testSetupBrokerSendsHeartbeat(10);

    // Client  ------Heartbeat--->  Proxy  --------Heartbeat-->  Broker
    testSetupClientSendsHeartbeat(11);

    // session->disconnect(false) called (Set up later after session object is
    // created).
    // Client  <-----Close--------  Proxy                        Broker
    testSetupProxySendsCloseToClient(12);

    // Client  ------CloseOk----->  Proxy                        Broker
    // Client                       Proxy  --------Close------>  Broker
    testSetupClientSendsCloseOk(13);

    // Client                       Proxy  <-------CloseOk-----  Broker
    // Proxy snaps all TCP connections at this point.
    // NB: In reality the client/broker may beat us by snapping their end
    // before we get to close ourselves.
    testSetupBrokerRespondsCloseOk(14);

    // After closing sockets any outstanding handlers will get aborted
    testSetupHandlersCleanedUp(15);
}

TEST_F(SessionTest, Connection_Then_Ping_Then_Disconnect)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnectWithDisconnect(&clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

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

TEST_F(SessionTest, BadClientHandshake)
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

    TestSocketState::State clientBase;
    clientBase.d_local  = makeEndpoint("1.2.3.4", 32000);
    clientBase.d_remote = makeEndpoint("3.4.5.6", 5672);
    clientBase.d_secure = false;

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker

    testSetupClientOpenWithoutTune(4);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Run the tests through to completion
    driveTo(4);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, BadServerHandshake)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    // Perform the 'TLS' handshake (no-op for tests), will just invoke the
    // completion handler
    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    testSetupClientOpen(4);

    // Client                       Proxy  <----TCP CONNECT---->  Broker
    testSetupProxyConnect(5, &clientBase);

    // Client                       Proxy  <-----HANDSHAKE----->  Broker
    // Client                       Proxy  -----AMQP Header---->  Broker
    testSetupProxySendsProtocolHeader(6);

    // not per protocol spec

    // Client                       Proxy  <-------OpenOk------  Broker
    testSetupProxyOutOfOrderOpen(7);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Run the tests through to completion
    driveTo(8);

    EXPECT_TRUE(session->finished());
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

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

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
        .WillOnce(DoAll(SetArgPointee<0>(cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    auto protocolHeader = std::vector<uint8_t>(
        Constants::protocolHeader(),
        Constants::protocolHeader() + Constants::protocolHeaderLength());

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnectWithDisconnect(&clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Override the expectation for proxy protocol
    d_clientState.expect(5, [&session, &backendPP](const auto &items) {
        auto data = filterVariant<Data>(items);

        ASSERT_EQ(data.size(), 1);
        auto generatedPPHeader = session->getProxyProtocolHeader(&backendPP);
        std::vector<uint8_t> generatedPPHeaderVec(
            generatedPPHeader.data(),
            generatedPPHeader.data() + generatedPPHeader.length());
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

struct MockDnsResolver {
    MOCK_METHOD3(resolve,
                 boost::system::error_code(
                     std::vector<boost::asio::ip::tcp::endpoint> *,
                     const std::string &,
                     const std::string &));
};

TEST_F(SessionTest, Connect_Multiple_Dns)
{
    using TcpEndpoint = boost::asio::ip::tcp::endpoint;
    using IpAddress   = boost::asio::ip::address;

    // Set up the only backend to resolve to two different addresses. This will
    // then be told to fail the first at connect and go onto the second
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(TcpEndpoint(IpAddress::from_string("::1"), 5672));
    resolveResult.push_back(
        TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672));

    boost::system::error_code goodErrorCode;
    MockDnsResolver           mockDns;
    EXPECT_CALL(mockDns, resolve(_, "localhost", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::OverrideFunctionGuard dnsguard(
        std::bind(&MockDnsResolver::resolve,
                  &mockDns,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));

    Backend singularBackend(
        "backend1", "dc1", "localhost", "127.0.0.1", 5672, false, false, true);
    std::vector<BackendSet::Partition> partitions;
    partitions.push_back(BackendSet::Partition{&singularBackend});

    d_cm = std::make_shared<ConnectionManager>(
        std::make_shared<BackendSet>(partitions), &d_robinSelector);

    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 1234)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 32000)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("3.4.5.6", 5672)))
        .WillRepeatedly(Return(std::string("host2")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("0.0.0.0", 0)))
        .WillRepeatedly(Return(std::string("<invalid>")));

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

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpen(4);

    d_clientState.pushItem(
        5, ConnectComplete(boost::asio::error::connection_refused));
    d_clientState.expect(5, [](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 0);
    });

    int step = 6;
    testSetupProxyConnect(step++, &clientBase);
    testSetupProxySendsProtocolHeader(step++);
    testSetupProxySendsStartOk(
        step++, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);
    testSetupProxyOpen(step++);
    testSetupProxyPassOpenOkThrough(step++);
    testSetupBrokerSendsHeartbeat(step++);
    testSetupClientSendsHeartbeat(step++);
    testSetupProxySendsCloseToClient(step++);
    testSetupClientSendsCloseOk(step++);
    testSetupBrokerRespondsCloseOk(step++);
    testSetupHandlersCleanedUp(step++);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Graceful disconnect after the heartbeats
    d_serverState.pushItem(13,
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

TEST_F(SessionTest, Failover_Dns_Failure)
{
    // Set up failover testing via three backends:
    //
    //  - Backend 1 will fail at the resolution step, and will failover to next
    //    backend
    //  - Backend 2 will succeed at resolution step, giving two different
    //    endpoints to try. Both backends will be failed at the async_connect
    //    step. This tests failing over through all the DNS steps and onto the
    //    next Backend
    //  - Backend 3 will succeed

    using TcpEndpoint = boost::asio::ip::tcp::endpoint;
    using IpAddress   = boost::asio::ip::address;

    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(TcpEndpoint(IpAddress::from_string("::1"), 5672));
    resolveResult.push_back(
        TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672));

    boost::system::error_code goodErrorCode;
    MockDnsResolver           mockDns;
    EXPECT_CALL(mockDns, resolve(_, "backend1", "5672"))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(resolveResult),
                        Return(boost::asio::error::access_denied)));
    EXPECT_CALL(mockDns, resolve(_, "backend2", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));
    EXPECT_CALL(mockDns, resolve(_, "backend3", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::OverrideFunctionGuard dnsguard(
        std::bind(&MockDnsResolver::resolve,
                  &mockDns,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));

    Backend backend1(
        "backend1", "dc1", "backend1", "127.0.0.1", 5672, false, false, true);
    Backend backend2(
        "backend2", "dc1", "backend2", "127.0.1.1", 5672, false, false, true);
    Backend backend3(
        "backend3", "dc1", "backend3", "127.0.1.1", 5672, false, false, true);
    std::vector<BackendSet::Partition> partitions;
    partitions.push_back(
        BackendSet::Partition{&backend1, &backend2, &backend3});

    d_cm = std::make_shared<ConnectionManager>(
        std::make_shared<BackendSet>(partitions), &d_robinSelector);

    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    EXPECT_CALL(*d_mapper, prime(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("2.3.4.5", 2345)))
        .WillRepeatedly(Return(std::string("host1")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 1234)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("1.2.3.4", 32000)))
        .WillRepeatedly(Return(std::string("host0")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("3.4.5.6", 5672)))
        .WillRepeatedly(Return(std::string("host2")));
    EXPECT_CALL(*d_mapper, mapToHostname(makeEndpoint("0.0.0.0", 0)))
        .WillRepeatedly(Return(std::string("<invalid>")));

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

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpen(4);

    d_clientState.pushItem(
        5, ConnectComplete(boost::asio::error::connection_refused));
    d_clientState.expect(5, [](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 0);
    });

    d_clientState.pushItem(
        6, ConnectComplete(boost::asio::error::connection_refused));
    d_clientState.expect(6, [](const auto &items) {
        auto data = filterVariant<Data>(items);
        ASSERT_EQ(data.size(), 0);
    });

    int step = 7;
    testSetupProxyConnect(step++, &clientBase);
    testSetupProxySendsProtocolHeader(step++);
    testSetupProxySendsStartOk(
        step++, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);
    testSetupProxyOpen(step++);
    testSetupProxyPassOpenOkThrough(step++);
    testSetupBrokerSendsHeartbeat(step++);
    testSetupClientSendsHeartbeat(step++);
    testSetupProxySendsCloseToClient(step++);
    testSetupClientSendsCloseOk(step++);
    testSetupBrokerRespondsCloseOk(step++);
    testSetupHandlersCleanedUp(step++);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Graceful disconnect after the heartbeats
    d_serverState.pushItem(14,
                           Func([&session] { session->disconnect(false); }));

    // Lastly, check it's elligible to be deleted
    d_serverState.pushItem(
        18, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_CLEANLY);
        }));

    // Run the tests through to completion
    driveTo(18);
}

TEST_F(SessionTest, Connection_Then_Ping_Then_Force_Disconnect)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpen(4);
    testSetupProxyConnect(5, &clientBase);
    testSetupProxySendsProtocolHeader(6);
    testSetupProxySendsStartOk(7, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);
    testSetupProxyOpen(8);
    testSetupProxyPassOpenOkThrough(9);
    testSetupBrokerSendsHeartbeat(10);
    testSetupClientSendsHeartbeat(11);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Force disconnect after the heartbeats
    d_serverState.pushItem(12,
                           Func([&session] { session->disconnect(true); }));

    // Lastly, check it's elligible to be deleted
    d_serverState.pushItem(
        13, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_PROXY);
        }));

    // Run the tests through to completion
    driveTo(13);
}

TEST_F(SessionTest, Connection_Then_Ping_Then_Backend_Disconnect)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpen(4);
    testSetupProxyConnect(5, &clientBase);
    testSetupProxySendsProtocolHeader(6);
    testSetupProxySendsStartOk(7, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);
    testSetupProxyOpen(8);
    testSetupProxyPassOpenOkThrough(9);
    testSetupBrokerSendsHeartbeat(10);
    testSetupClientSendsHeartbeat(11);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Disconnect only backend side
    d_serverState.pushItem(12,
                           Func([&session] { session->backendDisconnect(); }));

    d_clientState.expect(12, [this](const auto &items) {
        EXPECT_THAT(items,
                    Contains(VariantWith<Call>(Call("async_shutdown"))));
        EXPECT_THAT(items, Contains(VariantWith<Call>(Call("close"))));
    });
    d_serverState.expect(12, [this](const auto &items) {
        EXPECT_THAT(items,
                    Not(Contains(VariantWith<Call>(Call("async_shutdown")))));
        EXPECT_THAT(items, Not(Contains(VariantWith<Call>(Call("close")))));
    });

    // Lastly, check it's NOT elligible to be deleted
    d_serverState.pushItem(
        13, Func([&session] { EXPECT_FALSE(session->finished()); }));

    // Run the tests through to completion
    driveTo(13);
}

TEST_F(SessionTest, Authorized_Client_Test)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    std::string             modifiedMechanism   = "TEST_MECHANISM";
    std::string             modifiedCredentials = "credentials";
    std::string             reason              = "Authorized test client";
    authproto::AuthResponse authResponseData;
    authResponseData.set_result(authproto::AuthResponse::ALLOW);
    authResponseData.set_reason(reason);
    authproto::SASL *saslPtr = authResponseData.mutable_authdata();
    saslPtr->set_authmechanism(modifiedMechanism);
    saslPtr->set_credentials(modifiedCredentials);
    EXPECT_CALL(*d_mockAuthIntercept, authenticate(_, _))
        .WillOnce(InvokeArgument<1>(authResponseData));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpen(4);
    testSetupProxyConnect(5, &clientBase);
    testSetupProxySendsProtocolHeader(6);
    methods::StartOk overriddenStartOk = clientStartOk();
    overriddenStartOk.setAuthMechanism(modifiedMechanism);
    overriddenStartOk.setCredentials(modifiedCredentials);
    overriddenStartOk.properties().pushField("amqpprox_auth",
                                             FieldValue('S', reason));
    testSetupProxySendsStartOk(
        7, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000, overriddenStartOk);
    testSetupProxyOpen(8);
    testSetupProxyPassOpenOkThrough(9);
    testSetupBrokerSendsHeartbeat(10);
    testSetupClientSendsHeartbeat(11);
    testSetupProxySendsCloseToClient(12);
    testSetupClientSendsCloseOk(13);
    testSetupBrokerRespondsCloseOk(14);
    testSetupHandlersCleanedUp(15);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(
        std::move(clientSocket), std::move(serverSocket), d_mockAuthIntercept);

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

TEST_F(
    SessionTest,
    Unauthorized_Client_Test_Without_Authentication_Failure_Close_Capability)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    authproto::AuthResponse authResponseData;
    authResponseData.set_result(authproto::AuthResponse::DENY);
    authResponseData.set_reason("Unauthorized test client");
    EXPECT_CALL(*d_mockAuthIntercept, authenticate(_, _))
        .WillOnce(InvokeArgument<1>(authResponseData));

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

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    testSetupClientStartOk(3);
    testSetupClientOpenWithProxyClose(4);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(
        std::move(clientSocket), std::move(serverSocket), d_mockAuthIntercept);

    session->start();

    // Run the tests through to completion
    driveTo(5);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest,
       Unauthorized_Client_Test_With_Authentication_Failure_Close_Capability)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    authproto::AuthResponse authResponseData;
    authResponseData.set_result(authproto::AuthResponse::DENY);
    authResponseData.set_reason("Unauthorized test client");
    EXPECT_CALL(*d_mockAuthIntercept, authenticate(_, _))
        .WillOnce(InvokeArgument<1>(authResponseData));

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

    testSetupServerHandshake(1);
    testSetupClientSendsProtocolHeader(2);
    methods::StartOk overriddenStartOk = clientStartOk();
    FieldTable       clientProperties;
    auto             capabilitiesTable = std::make_shared<FieldTable>();
    capabilitiesTable->pushField(Constants::authenticationFailureClose(),
                                 FieldValue('t', true));
    clientProperties.pushField(Constants::capabilities(),
                               FieldValue('F', capabilitiesTable));
    overriddenStartOk.setClientProperties(clientProperties);
    testSetupClientStartOk(3, overriddenStartOk);

    std::shared_ptr<methods::Close> closeMethodPtr =
        std::make_shared<methods::Close>();
    closeMethodPtr->setReply(Reply::Codes::access_refused,
                             "Unauthorized test client",
                             methods::StartOk::classType(),
                             methods::StartOk::methodType());
    testSetupClientOpenWithProxyClose(4, closeMethodPtr);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(
        std::move(clientSocket), std::move(serverSocket), d_mockAuthIntercept);

    session->start();

    // Run the tests through to completion
    driveTo(5);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, Forward_Received_Close_Method_To_Client_During_Handshake)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    testSetupClientOpen(4);

    // Client                       Proxy  <----TCP CONNECT---->  Broker
    testSetupProxyConnect(5, &clientBase);

    // Client                       Proxy  <-----HANDSHAKE----->  Broker
    // Client                       Proxy  -----AMQP Header---->  Broker
    testSetupProxySendsProtocolHeader(6);

    // Client                       Proxy  <-------Start--------  Broker
    // Client                       Proxy  --------StartOk----->  Broker
    testSetupProxySendsStartOk(7, "host1", 2345, LOCAL_HOSTNAME, 1234, 32000);

    // This is the important part of the test - ensure that if the broker sends
    // a Close during handshake, proxy pass this connection Close method to the
    // client Client  <-------Close------- Proxy  <-------Close--------  Broker
    testSetupProxyForwardsBrokerClose(8);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    // Run the tests through to completion
    driveTo(9);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, Close_Connection_No_Broker_Mapping)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::NO_BACKEND)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    // Client  <-----Close--------  Proxy                         Broker
    std::shared_ptr<methods::Close> closeMethodPtr =
        std::make_shared<methods::Close>();
    closeMethodPtr->setReply(Reply::Codes::resource_error,
                             "No known broker mapping for vhost ");
    testSetupClientOpenWithProxyClose(4, closeMethodPtr);

    session->start();

    // Run the tests through to completion
    driveTo(5);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, Close_Limited_Connection)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::LIMIT)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    testSetupServerHandshake(1);

    // Read a protocol header from the client and reply with Start method
    // Client  ----AMQP Header--->  Proxy                         Broker
    // Client  <-----Start--------  Proxy                         Broker
    testSetupClientSendsProtocolHeader(2);

    // Client  ------StartOk----->  Proxy                         Broker
    // Client  <-----Tune---------  Proxy                         Broker
    testSetupClientStartOk(3);

    // Client  ------TuneOk------>  Proxy                         Broker
    // Client  ------Open-------->  Proxy                         Broker
    // Client  <-----Close--------  Proxy                         Broker
    std::shared_ptr<methods::Close> closeMethodPtr =
        std::make_shared<methods::Close>();
    std::string vhostName;
    closeMethodPtr->setReply(Reply::Codes::resource_error,
                             "The connection for " + vhostName +
                                 ", is limited by proxy.");
    testSetupClientOpenWithProxyClose(4, closeMethodPtr);

    session->start();

    // Run the tests through to completion
    driveTo(5);

    EXPECT_TRUE(session->finished());
}

TEST_F(SessionTest, Printing_Breathing_Test)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnectWithDisconnect(&clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    std::stringstream oss;
    // Print the Session then graceful disconnect after the heartbeats
    d_serverState.pushItem(12, Func([&session, &oss] {
                               session->print(oss);
                               session->disconnect(false);
                           }));

    // Lastly, check it's elligible to be deleted
    d_serverState.pushItem(
        17, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_CLEANLY);
        }));

    // Run the tests through to completion
    driveTo(17);

    // Check that something was printed, NB we do not expect any exact format
    EXPECT_NE(oss.str().length(), 0);
}

TEST_F(SessionTest, Pause_Disconnects_Previously_Established_Connection)
{
    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runStandardConnect(&clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    session->start();

    d_serverState.pushItem(16, Func([&session] { session->pause(); }));

    driveTo(16);

    // Unpause
    d_serverState.pushItem(17, Func([&session] { session->unpause(); }));

    // Check unpause disconnected us
    d_serverState.pushItem(
        18, Func([&session] {
            EXPECT_TRUE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::DISCONNECTED_PROXY);
        }));

    // Run the tests through to completion
    driveTo(18);
}

TEST_F(SessionTest,
       Pause_Before_Connection_Holds_Before_Broker_Connect_Resumes_On_Unpause)
{
    TestSocketState::State base, clientBase;
    testSetupHostnameMapperForServerClientBase(base, clientBase);

    // Initialise the state
    d_serverState.pushItem(0, base);
    driveTo(0);

    runConnectToClientOpen(&clientBase);

    std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_client, false);
    std::shared_ptr<MaybeSecureSocketAdaptor<>> serverSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_server, false);
    auto session = makeSession(clientSocket, serverSocket);

    // Emulate `VhostEstablishedPauser` but pause everything
    bool                    paused = false;
    EventSubscriptionHandle vhostPauser =
        d_eventSource.connectionVhostEstablished().subscribe(
            [&session, &paused](uint64_t, const std::string &) {
                session->pause();
                paused = true;
            });

    session->start();

    d_serverState.pushItem(
        7, Func([&session, &paused] {
            EXPECT_TRUE(paused);

            EXPECT_FALSE(session->finished());
            EXPECT_TRUE(session->state().getPaused());
            EXPECT_TRUE(session->state().getReadyToConnectOnUnpause());
        }));

    {
        // Ensure acquireConnection isn't called
        EXPECT_CALL(d_selector, acquireConnection(_, _)).Times(0);

        driveTo(7);
    }

    // Unpause
    d_serverState.pushItem(8, Func([&session] { session->unpause(); }));

    runBrokerHandshake(&clientBase, 9);

    // Client  <-----OpenOk-------  Proxy  <-------OpenOk------  Broker
    testSetupProxyPassOpenOkThrough(13);

    // Check we're connected (or, "not_disconnected")
    d_serverState.pushItem(
        15, Func([&session] {
            EXPECT_FALSE(session->finished());
            EXPECT_EQ(session->state().getDisconnectType(),
                      SessionState::DisconnectType::NOT_DISCONNECTED);
        }));

    EXPECT_CALL(d_selector, acquireConnection(_, _))
        .WillOnce(DoAll(SetArgPointee<0>(d_cm),
                        Return(SessionState::ConnectionStatus::SUCCESS)));

    // Run the tests through to completion
    driveTo(16);
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
: d_ioContext()
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
, d_dnsResolver(d_ioContext)
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
, d_protocolHeader(Constants::protocolHeader(),
                   Constants::protocolHeader() +
                       Constants::protocolHeaderLength())
, d_step(0)
, d_authIntercept(std::make_shared<DefaultAuthIntercept>(d_ioContext))
, d_mockAuthIntercept(
      std::make_shared<AuthInterceptInterfaceMock>(d_ioContext))
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
            bool ioloopHadWork = d_ioContext.run() > 0;
            d_ioContext.restart();
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
    result.setReply(Reply::Codes::reply_success, "OK");
    return result;
}
