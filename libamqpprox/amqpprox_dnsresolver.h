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
#ifndef BLOOMBERG_AMQPPROX_DNSRESOLVER
#define BLOOMBERG_AMQPPROX_DNSRESOLVER

#include <boost/asio.hpp>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Encapsulates asynchronous DNS resolution
 *
 * This component provides an asynchronous callback for resolving DNS names
 * into (potentially) multiple endpoints. It includes a cache, which serves
 * three overall purposes:
 * 1. Faster resolution times
 * 2. Protect downstream resolves from connection spikes
 * 3. Enable testing of other components, as the cache can prevent the DNS
 *    resolution.
 */
class DNSResolver {
    using TcpEndpoint = boost::asio::ip::tcp::endpoint;

    boost::asio::io_service &d_ioService;

  public:
    explicit DNSResolver(boost::asio::io_service &ioService);

    template <typename ResolveCallback>
    void resolve(std::string_view       query_host,
                 std::string_view       query_service,
                 const ResolveCallback &callback);

    void setCacheTimeout(int timeoutMs);

    void setCachedResolution(std::string_view           query_host,
                             std::string_view           query_service,
                             std::vector<TcpEndpoint> &&resolution);

    void clearCachedResolution(std::string_view query_host,
                               std::string_view query_service);
  private:

    void cleanupCache();
};

}
}

#endif
