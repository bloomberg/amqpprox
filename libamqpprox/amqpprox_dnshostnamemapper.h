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
#ifndef BLOOMBERG_AMQPPROX_DNSHOSTNAMEMAPPER
#define BLOOMBERG_AMQPPROX_DNSHOSTNAMEMAPPER

#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <unordered_map>

#include <amqpprox_hostnamemapper.h>

namespace Bloomberg {
namespace amqpprox {

class DNSHostnameMapper : public HostnameMapper {
    std::unordered_map<std::string, std::string> d_hostnameMap;
    mutable boost::shared_mutex                  d_lg;

  public:
    DNSHostnameMapper();

    void prime(boost::asio::io_service &                             ioService,
               std::initializer_list<boost::asio::ip::tcp::endpoint> endpoints)
        override;

    std::string mapToHostname(
        const boost::asio::ip::tcp::endpoint &endpoint) const override;
};

}
}

#endif
