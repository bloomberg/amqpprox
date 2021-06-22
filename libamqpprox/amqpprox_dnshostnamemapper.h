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
#ifndef BLOOMBERG_AMQPPROX_DNSHOSTNAMEMAPPER
#define BLOOMBERG_AMQPPROX_DNSHOSTNAMEMAPPER

#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <unordered_map>

#include <amqpprox_hostnamemapper.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief DNSHostnameMapper facilitates reverse lookups based on IP address to
 * hostname, implements the HostnameMapper interface
 */
class DNSHostnameMapper : public HostnameMapper {
    std::unordered_map<std::string, std::string> d_hostnameMap;
    mutable boost::shared_mutex                  d_lg;

  public:
    DNSHostnameMapper();

    /**
     * \brief prime the cache of hostnames with a list of endpoints
     * \param ioService handle to the boost asio service
     * \param endpoints list of endpoints to prime the cache with
     */
    void prime(boost::asio::io_service &                             ioService,
               std::initializer_list<boost::asio::ip::tcp::endpoint> endpoints)
        override;

    /**
     * \brief sychronously reverse lookup the ip address to hostname based on
     * cached data
     * \param endpoint - ip address
     * \returns hostname if the ip address is
     * in the cache (hit), returns the endpoint as a string e.g. "127.0.0.1" on
     * miss
     */
    std::string mapToHostname(
        const boost::asio::ip::tcp::endpoint &endpoint) const override;
};

}
}

#endif
