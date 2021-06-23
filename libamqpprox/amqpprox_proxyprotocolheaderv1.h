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
#ifndef BLOOMBERG_PROXY_PROTOCOL_HEADER_V1
#define BLOOMBERG_PROXY_PROTOCOL_HEADER_V1

#include <string>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Represents a proxy protocol V1 header.
 *
 * The proxy protocol V1 header consists of one line of US-ASCII text sent
 * immediately and at once upon the connection establishment and prepended
 * before any data flowing from the sender to the receiver.
 *
 * PROXY <INET-PROTOCOL> <SRC-IP> <DEST-IP> <SRC-PORT> <DEST-PORT>\r\n
 * where INET-PROTOCOL is one of TCP4, TCP6 or UNKNOWN
 *
 * More details here
 * https://www.haproxy.org/download/1.8/doc/proxy-protocol.txt
 */
class ProxyProtocolHeaderV1 {
  public:
    enum class InetProtocol { TCP4, TCP6, UNKNOWN };

  private:
    InetProtocol d_inetProtocol;
    std::string  d_sourceIp;
    std::string  d_destinationIp;
    int          d_sourcePort;
    int          d_destinationPort;

  public:
    // CREATORS
    ProxyProtocolHeaderV1();

    ProxyProtocolHeaderV1(InetProtocol       inetProtocol,
                          const std::string &sourceIp,
                          const std::string &destinationIp,
                          int                sourcePort,
                          int                destinationPort);

    // ACCESSORS
    inline InetProtocol       inetProtocol() const;
    inline const std::string &sourceIp() const;
    inline const std::string &destinationIp() const;
    inline int                sourcePort() const;
    inline int                destinationPort() const;
};

inline ProxyProtocolHeaderV1::InetProtocol
ProxyProtocolHeaderV1::inetProtocol() const
{
    return d_inetProtocol;
}

inline const std::string &ProxyProtocolHeaderV1::sourceIp() const
{
    return d_sourceIp;
}

inline const std::string &ProxyProtocolHeaderV1::destinationIp() const
{
    return d_destinationIp;
}

inline int ProxyProtocolHeaderV1::sourcePort() const
{
    return d_sourcePort;
}

inline int ProxyProtocolHeaderV1::destinationPort() const
{
    return d_destinationPort;
}

std::ostream &operator<<(std::ostream &               os,
                         const ProxyProtocolHeaderV1 &header);

}  // amqpprox namespace
}  // Bloomberg namespace

#endif
