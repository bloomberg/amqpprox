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
#include <amqpprox_backend.h>
#include <amqpprox_constants.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

Backend::Backend(const std::string &name,
                 const std::string &datacenterTag,
                 const std::string &host,
                 const std::string &ip,
                 int                port,
                 bool               proxyEnabled,
                 bool               tlsEnabled)
: d_name(name)
, d_datacenterTag(datacenterTag)
, d_host(host)
, d_ip(ip)
, d_port(port)
, d_proxyProtocolEnabled(proxyEnabled)
, d_tlsEnabled(tlsEnabled)
{
}

Backend::Backend()
: d_name("")
, d_datacenterTag("")
, d_host("")
, d_ip("")
, d_port(0)
, d_proxyProtocolEnabled(false)
, d_tlsEnabled(false)
{
}

std::ostream &operator<<(std::ostream &os, const Backend &backend)
{
    os << backend.name() << " (" << backend.datacenterTag()
       << "): " << backend.host() << " " << backend.ip() << ":"
       << backend.port();
    if (backend.proxyProtocolEnabled()) {
        os << " " << Constants::proxyProtocolV1Enabled();
    }
    if (backend.tlsEnabled()) {
        os << " TLS";
    }
    return os;
}

}
}
