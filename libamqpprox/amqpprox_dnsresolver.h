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

#include <functional>
#include <unordered_map>

#include <stdint.h>

namespace Bloomberg {
namespace amqpprox {

struct PairHash {
    size_t operator()(const std::pair<std::string, std::string> &obj)
    {
        return std::hash<std::string>{}(std::get<0>(obj)) ^
               std::hash<std::string>{}(std::get<1>(obj));
    }
};

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
    using CacheType   = std::unordered_map<std::pair<std::string, std::string>,
                                         std::vector<TcpEndpoint>,
                                         PairHash>;

    boost::asio::io_service &      d_ioService;
    boost::asio::ip::tcp::resolver d_resolver;
    boost::asio::steady_timer      d_timer;
    std::atomic<uint32_t>          d_cacheTimeout;
    std::atomic<bool>              d_cacheTimerRunning;
    std::mutex                     d_cacheLock;
    CacheType                      d_cache;

  public:
    explicit DNSResolver(boost::asio::io_service &ioService);
    ~DNSResolver();

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

    void startCleanupTimer();
    void stopCleanupTimer();

  private:
    void cleanupCache(const boost::system::error_code &ec);
};

template <typename ResolveCallback>
void DNSResolver::resolve(std::string_view       query_host,
                          std::string_view       query_service,
                          const ResolveCallback &callback)
{
    using endpointIt = boost::asio::ip::tcp::resolver::iterator;
    std::string host = std::string(query_host);
    std::string service = std::string(query_service);
    boost::asio::ip::tcp::resolver::query query(host, service);

    // TODO use the cache here?

    auto resolveCb = [this, callback](const boost::system::error_code &ec,
                                           endpointIt        endpoint) {
        std::vector<TcpEndpoint>  endpoints;
        endpointIt end;
        while (endpoint != end) {
            endpoints.push_back(*endpoint);
            ++endpoint;
        }
        callback(ec, endpoints);
    };
    d_resolver.async_resolve(query, resolveCb);
}

}
}

#endif
