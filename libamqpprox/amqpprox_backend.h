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
#ifndef BLOOMBERG_AMQPPROX_BACKEND
#define BLOOMBERG_AMQPPROX_BACKEND

#include <iosfwd>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Backend {
    std::string d_name;
    std::string d_datacenterTag;
    std::string d_host;
    std::string d_ip;
    int         d_port;
    bool        d_proxyProtocolEnabled;
    bool        d_tlsEnabled;

  public:
    Backend(const std::string &name,
            const std::string &datacenterTag,
            const std::string &host,
            const std::string &ip,
            int                port,
            bool               proxyProtocolEnabled = false,
            bool               tlsEnabled           = false);

    Backend();

    inline const std::string &host() const;
    inline const std::string &ip() const;
    inline int                port() const;
    inline const std::string &datacenterTag() const;
    inline const std::string &name() const;
    inline bool               proxyProtocolEnabled() const;
    inline bool               tlsEnabled() const;
};

inline const std::string &Backend::host() const
{
    return d_host;
}

inline const std::string &Backend::ip() const
{
    return d_ip;
}

inline int Backend::port() const
{
    return d_port;
}

inline const std::string &Backend::datacenterTag() const
{
    return d_datacenterTag;
}

inline const std::string &Backend::name() const
{
    return d_name;
}

inline bool Backend::proxyProtocolEnabled() const
{
    return d_proxyProtocolEnabled;
}

inline bool Backend::tlsEnabled() const
{
    return d_tlsEnabled;
}

std::ostream &operator<<(std::ostream &os, const Backend &backend);

}
}

#endif
