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

#include <amqpprox_defaultauthintercept.h>

#include <authrequest.pb.h>
#include <authresponse.pb.h>

#include <gmock/gmock.h>

#include <iostream>

#include <boost/asio.hpp>

using namespace Bloomberg;
using namespace amqpprox;
using Bloomberg::amqpprox::DefaultAuthIntercept;

TEST(DefaultAuthIntercept, Breathing)
{
    boost::asio::io_context ioContext;
    DefaultAuthIntercept    defaultAuth(ioContext);
    ioContext.run();
}

TEST(DefaultAuthIntercept, Authenticate)
{
    boost::asio::io_context ioContext;
    DefaultAuthIntercept    defaultAuth(ioContext);
    auto responseCb = [](const authproto::AuthResponse &authResponseData) {
        ASSERT_EQ(authResponseData.result(), authproto::AuthResponse::ALLOW);
        ASSERT_EQ(authResponseData.reason(),
                  "Default route auth used - always allow");
    };
    defaultAuth.authenticate(authproto::AuthRequest(), responseCb);
    ioContext.run();
}

TEST(DefaultAuthIntercept, Print)
{
    boost::asio::io_context ioContext;
    DefaultAuthIntercept    defaultAuth(ioContext);
    ioContext.run();
    std::ostringstream oss;
    defaultAuth.print(oss);
    EXPECT_EQ(oss.str(),
              "All connections are authorised to route to any vhost. No auth "
              "service requests will be made.\n");
}
