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

#include <amqpprox_dnsresolver.h>

#include <chrono>
#include <gmock/gmock-generated-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using Bloomberg::amqpprox::DNSResolver;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using IpAddress   = boost::asio::ip::address;
using namespace testing;

TEST(DNSResolver, Breathing)
{
    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    ioContext.run();
}

TEST(DNSResolver, StartStopCleanup)
{
    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    resolver.stopCleanupTimer();
    ioContext.run();
}

struct MockDnsResolver {
    MOCK_METHOD3(resolve,
                 boost::system::error_code(
                     std::vector<boost::asio::ip::tcp::endpoint> *,
                     const std::string &,
                     const std::string &));
};

TEST(DNSResolver, Override_And_Return)
{
    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(local_ipv6);
    resolveResult.push_back(local_ipv4);

    MockDnsResolver           mockDns;
    boost::system::error_code goodErrorCode;
    EXPECT_CALL(mockDns, resolve(_, "test1", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb = [local_ipv4,
               local_ipv6](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };
    resolver.resolve("test1", "5672", cb);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Cache_Removes_Multiple_Resolutions)
{
    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(local_ipv6);
    resolveResult.push_back(local_ipv4);

    MockDnsResolver           mockDns;
    boost::system::error_code goodErrorCode;
    EXPECT_CALL(mockDns, resolve(_, "test1", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb = [local_ipv4,
               local_ipv6](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };

    // Make two requests, even though underlying Mock DNS resolution expecting
    // only one.
    resolver.resolve("test1", "5672", cb);
    resolver.resolve("test1", "5672", cb);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Multiple_Resolutions_Needed_After_Cache_Cleanup)
{
    using namespace std::chrono_literals;

    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(local_ipv6);
    resolveResult.push_back(local_ipv4);

    MockDnsResolver           mockDns;
    boost::system::error_code goodErrorCode;
    EXPECT_CALL(mockDns, resolve(_, "test1", "5672"))
        .Times(2)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    auto                    cb = [local_ipv4,
               local_ipv6](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };

    // First request, prime the cache
    resolver.resolve("test1", "5672", cb);

    // Start the cache cleanup and ensure it runs, then break after it's had
    // the chance
    resolver.setCacheTimeout(1);
    resolver.startCleanupTimer();
    ioContext.run_for(50ms);

    // Second request should be cache cold, so expectation is 2 times
    resolver.resolve("test1", "5672", cb);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Independent_Resolutions_Get_Cached_Indpendently)
{
    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult1;
    resolveResult1.push_back(local_ipv6);
    resolveResult1.push_back(local_ipv4);

    auto ip2 = TcpEndpoint(IpAddress::from_string("127.1.1.1"), 5673);
    std::vector<TcpEndpoint> resolveResult2;
    resolveResult2.push_back(ip2);

    MockDnsResolver           mockDns;
    boost::system::error_code goodErrorCode;

    EXPECT_CALL(mockDns, resolve(_, "test1", "5672"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult1), Return(goodErrorCode)));

    EXPECT_CALL(mockDns, resolve(_, "test2", "5673"))
        .Times(1)
        .WillOnce(
            DoAll(SetArgPointee<0>(resolveResult2), Return(goodErrorCode)));

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb1 = [local_ipv4,
                local_ipv6](const boost::system::error_code &ec,
                            const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };
    auto cb2 = [ip2](const boost::system::error_code &ec,
                     const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 1);
        EXPECT_THAT(endpoints, UnorderedElementsAre(ip2));
    };

    // Make two requests for each end point, even though underlying Mock DNS
    // resolution expecting only one.
    resolver.resolve("test1", "5672", cb1);
    resolver.resolve("test1", "5672", cb1);
    resolver.resolve("test2", "5673", cb2);
    resolver.resolve("test2", "5673", cb2);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, No_Underlying_Call_When_Cached)
{
    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult1;
    resolveResult1.push_back(local_ipv6);
    resolveResult1.push_back(local_ipv4);

    MockDnsResolver mockDns;
    // No calls EXPECT'd here

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb1 = [local_ipv4,
                local_ipv6](const boost::system::error_code &ec,
                            const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };

    resolver.setCachedResolution("test1", "5672", std::move(resolveResult1));
    resolver.resolve("test1", "5672", cb1);
    resolver.resolve("test1", "5672", cb1);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Cache_Clear_Means_Multiple_Resolutions)
{
    auto local_ipv6 = TcpEndpoint(IpAddress::from_string("::1"), 5672);
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(local_ipv6);
    resolveResult.push_back(local_ipv4);

    MockDnsResolver           mockDns;
    boost::system::error_code goodErrorCode;
    EXPECT_CALL(mockDns, resolve(_, "test1", "5672"))
        .Times(2)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(resolveResult), Return(goodErrorCode)));

    DNSResolver::setOverrideFunction(std::bind(&MockDnsResolver::resolve,
                                               &mockDns,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb = [local_ipv4,
               local_ipv6](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };

    // Make two requests, even though underlying Mock DNS resolution expecting
    // only one.
    resolver.resolve("test1", "5672", cb);
    resolver.clearCachedResolution("test1", "5672");
    resolver.resolve("test1", "5672", cb);
    resolver.stopCleanupTimer();
    ioContext.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Real_Resolver_For_IP)
{
    auto local_ipv4 = TcpEndpoint(IpAddress::from_string("127.0.0.1"), 5672);
    std::vector<TcpEndpoint> resolveResult;
    resolveResult.push_back(local_ipv4);

    boost::asio::io_context ioContext;
    DNSResolver             resolver(ioContext);
    resolver.startCleanupTimer();
    auto cb = [local_ipv4](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 1);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4));
    };
    resolver.resolve("127.0.0.1", "5672", cb);
    resolver.stopCleanupTimer();
    ioContext.run();
}
