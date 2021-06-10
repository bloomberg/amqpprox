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

#include "gmock/gmock-generated-matchers.h"
#include <amqpprox_dnsresolver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using Bloomberg::amqpprox::DNSResolver;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using IpAddress   = boost::asio::ip::address;
using namespace testing;

TEST(DNSResolver, Breathing)
{
    boost::asio::io_service ioService;
    DNSResolver             resolver(ioService);
    ioService.run();
}

TEST(DNSResolver, StartStopCleanup)
{
    boost::asio::io_service ioService;
    DNSResolver             resolver(ioService);
    resolver.startCleanupTimer();
    resolver.stopCleanupTimer();
    ioService.run();
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

    boost::asio::io_service ioService;
    DNSResolver             resolver(ioService);
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
    ioService.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}

TEST(DNSResolver, Multiple_Resolutions)
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

    boost::asio::io_service ioService;
    DNSResolver             resolver(ioService);
    resolver.startCleanupTimer();
    auto cb = [local_ipv4,
               local_ipv6](const boost::system::error_code &ec,
                           const std::vector<TcpEndpoint> & endpoints) {
        ASSERT_EQ(ec, boost::system::error_code());
        ASSERT_EQ(endpoints.size(), 2);
        EXPECT_THAT(endpoints, UnorderedElementsAre(local_ipv4, local_ipv6));
    };
    resolver.resolve("test1", "5672", cb);
    resolver.resolve("test1", "5672", cb);
    resolver.stopCleanupTimer();
    ioService.run();

    DNSResolver::setOverrideFunction(DNSResolver::OverrideFunction());
}
