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
#include <amqpprox_proxyprotocolheaderv1.h>

#include <sstream>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::ProxyProtocolHeaderV1;

TEST(ProxyProtocolHeaderV1, Unknown)
{
    ProxyProtocolHeaderV1 header = ProxyProtocolHeaderV1();

    std::stringstream headerStream;
    headerStream << header;

    EXPECT_EQ("PROXY UNKNOWN\r\n", headerStream.str());
}

TEST(ProxyProtocolHeaderV1, Tcp4)
{
    ProxyProtocolHeaderV1 header =
        ProxyProtocolHeaderV1(ProxyProtocolHeaderV1::InetProtocol::TCP4,
                              "192.168.1.1",
                              "192.168.1.2",
                              80,
                              81);

    std::stringstream headerStream;
    headerStream << header;

    EXPECT_EQ("PROXY TCP4 192.168.1.1 192.168.1.2 80 81\r\n",
              headerStream.str());
}

TEST(ProxyProtocolHeaderV1, Tcp6)
{
    ProxyProtocolHeaderV1 header = ProxyProtocolHeaderV1(
        ProxyProtocolHeaderV1::InetProtocol::TCP6, "::1", "::2", 80, 81);

    std::stringstream headerStream;
    headerStream << header;

    EXPECT_EQ("PROXY TCP6 ::1 ::2 80 81\r\n", headerStream.str());
}
