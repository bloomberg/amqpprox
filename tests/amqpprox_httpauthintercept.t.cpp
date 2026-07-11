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

#include <authrequest.pb.h>
#include <sasl.pb.h>

#include <gmock/gmock.h>

#include <iostream>

#include <boost/asio.hpp>

using namespace Bloomberg;
using namespace amqpprox;
using Bloomberg::amqpprox::HttpAuthIntercept;

TEST(HttpAuthIntercept, Breathing)
{
    boost::asio::io_context ioContext;
    DNSResolver             dnsResolver(ioContext);
    HttpAuthIntercept       authIntercept(
        ioContext, "localhost", "8080", "/target", &dnsResolver);
    ioContext.run();
}

TEST(HttpAuthIntercept, Print)
{
    boost::asio::io_context ioContext;
    DNSResolver             dnsResolver(ioContext);
    HttpAuthIntercept       authIntercept(
        ioContext, "localhost", "8080", "/target", &dnsResolver);
    ioContext.run();
    std::ostringstream oss;
    authIntercept.print(oss);
    EXPECT_EQ(oss.str(),
              "HTTP Auth service will be used to authn/authz client "
              "connections: http://localhost:8080/target\n");
}

TEST(HttpAuthIntercept, AuthRequestCarriesClientHostnameAndConnectionName)
{
    // The client's originating host and AMQP connection name are forwarded to
    // the auth gate as new proto3 fields. Verify they survive a serialize /
    // parse round-trip alongside the pre-existing fields.
    authproto::AuthRequest request;
    request.set_vhostname("my-vhost");
    request.set_clienthostname("client-host.example.com");
    request.set_connectionname("my-connection");
    authproto::SASL *sasl = request.mutable_authdata();
    sasl->set_authmechanism("PLAIN");
    sasl->set_credentials(std::string("\0user\0pass", 10));

    std::string serialized;
    ASSERT_TRUE(request.SerializeToString(&serialized));

    authproto::AuthRequest parsed;
    ASSERT_TRUE(parsed.ParseFromString(serialized));

    EXPECT_EQ(parsed.vhostname(), "my-vhost");
    EXPECT_EQ(parsed.clienthostname(), "client-host.example.com");
    EXPECT_EQ(parsed.connectionname(), "my-connection");
    EXPECT_EQ(parsed.authdata().authmechanism(), "PLAIN");
    EXPECT_EQ(parsed.authdata().credentials(),
              std::string("\0user\0pass", 10));
}
