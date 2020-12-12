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
#include <amqpprox_constants.h>
#include <amqpprox_proxyprotocolheaderv1.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

ProxyProtocolHeaderV1::ProxyProtocolHeaderV1(InetProtocol       inetProtocol,
                                             const std::string &sourceIp,
                                             const std::string &destinationIp,
                                             int                sourcePort,
                                             int destinationPort)
: d_inetProtocol(inetProtocol)
, d_sourceIp(sourceIp)
, d_destinationIp(destinationIp)
, d_sourcePort(sourcePort)
, d_destinationPort(destinationPort)
{
}

ProxyProtocolHeaderV1::ProxyProtocolHeaderV1()
: d_inetProtocol(InetProtocol::UNKNOWN)
, d_sourceIp("")
, d_destinationIp("")
, d_sourcePort(0)
, d_destinationPort(0)
{
}

std::ostream &operator<<(std::ostream &os, const ProxyProtocolHeaderV1 &header)
{
    os << Constants::proxyProtocolV1Identifier() << " ";
    switch (header.inetProtocol()) {
    case ProxyProtocolHeaderV1::InetProtocol::TCP4:
        os << Constants::inetTcp4() << " ";
        break;
    case ProxyProtocolHeaderV1::InetProtocol::TCP6:
        os << Constants::inetTcp6() << " ";
        break;
    default:
        os << Constants::inetUnknown() << Constants::cr();
        return os;
    }
    os << header.sourceIp() << " " << header.destinationIp() << " "
       << header.sourcePort() << " " << header.destinationPort()
       << Constants::cr();
    return os;
}

}  // amqpprox namespace
}  // Bloomberg namespace
