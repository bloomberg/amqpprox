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
#include <amqpprox_sessionstate.h>

#include <amqpprox_hostnamemapper.h>

#include <boost/asio.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg::amqpprox;
using namespace boost::asio;
using namespace ::testing;

namespace {

class MockHostnameMapper : public HostnameMapper {
  public:
    MOCK_METHOD2(
        prime,
        void(boost::asio::io_service &ioService,
             const std::initializer_list<boost::asio::ip::tcp::endpoint>
                 endpoint));
    MOCK_CONST_METHOD1(
        mapToHostname,
        std::string(const boost::asio::ip::tcp::endpoint &endpoint));
};

}

TEST(SessionState, noHostnameMapper)
{
    io_service ioService;

    SessionState s(nullptr);

    auto ip1  = "1.1.1.1";
    auto ip2  = "2.2.2.2";
    auto port = 42;

    s.setEgress(ioService,
                ip::tcp::endpoint(ip::address::from_string(ip1), port),
                ip::tcp::endpoint(ip::address::from_string(ip2), port));

    auto egress = s.getEgress();
    EXPECT_EQ(ip1, s.hostname(egress.first));
    EXPECT_EQ(ip2, s.hostname(egress.second));
}

TEST(SessionState, hostnameMapper)
{
    io_service ioService;

    auto         m = std::make_shared<MockHostnameMapper>();
    SessionState s(m);

    auto ip1  = "1.1.1.1";
    auto ip2  = "2.2.2.2";
    auto port = 42;

    EXPECT_CALL(*m, prime(_, _)).Times(1);
    s.setEgress(ioService,
                ip::tcp::endpoint(ip::address::from_string(ip1), port),
                ip::tcp::endpoint(ip::address::from_string(ip2), port));

    EXPECT_CALL(*m, mapToHostname(_))
        .WillOnce(Return(ip1))
        .WillOnce(Return(ip2));

    auto egress = s.getEgress();

    EXPECT_EQ(ip1, s.hostname(egress.first));
    EXPECT_EQ(ip2, s.hostname(egress.second));
}
