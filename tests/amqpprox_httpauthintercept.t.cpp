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

#include <amqpprox_httpauthintercept.h>

#include <gmock/gmock.h>

#include <iostream>

#include <boost/asio.hpp>

using namespace Bloomberg;
using namespace amqpprox;
using Bloomberg::amqpprox::HttpAuthIntercept;

TEST(HttpAuthIntercept, Breathing)
{
    boost::asio::io_service ioService;
    DNSResolver             dnsResolver(ioService);
    HttpAuthIntercept       authIntercept(
        ioService, "localhost", "8080", "/target", &dnsResolver);
    ioService.run();
}

TEST(HttpAuthIntercept, Print)
{
    boost::asio::io_service ioService;
    DNSResolver             dnsResolver(ioService);
    HttpAuthIntercept       authIntercept(
        ioService, "localhost", "8080", "/target", &dnsResolver);
    ioService.run();
    std::ostringstream oss;
    authIntercept.print(oss);
    EXPECT_EQ(oss.str(),
              "HTTP Auth service will be used to authn/authz client "
              "connections: http://localhost:8080/target\n");
}
