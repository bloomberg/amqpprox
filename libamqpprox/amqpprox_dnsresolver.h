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

#include <amqpprox_logging.h>

#include <boost/asio.hpp>

#include <functional>
#include <unordered_map>

#include <stdint.h>

namespace Bloomberg {
namespace amqpprox {

struct PairHash {
    size_t operator()(const std::pair<std::string, std::string> &obj) const
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
    using OverrideFunction =
        std::function<boost::system::error_code(std::vector<TcpEndpoint> *,
                                                const std::string &,
                                                const std::string &)>;

    boost::asio::io_service &      d_ioService;
    boost::asio::ip::tcp::resolver d_resolver;
    boost::asio::steady_timer      d_timer;
    std::atomic<uint32_t>          d_cacheTimeout;
    std::atomic<bool>              d_cacheTimerRunning;
    std::mutex                     d_cacheLock;
    CacheType                      d_cache;

    static OverrideFunction s_override;

  public:
    explicit DNSResolver(boost::asio::io_service &ioService);
    ~DNSResolver();

    template <typename ResolveCallback>
    void resolve(std::string_view       query_host,
                 std::string_view       query_service,
                 const ResolveCallback &callback);

    void setCacheTimeout(int timeoutMs);

    void setCachedResolution(const std::string &        query_host,
                             const std::string &        query_service,
                             std::vector<TcpEndpoint> &&resolution);

    void clearCachedResolution(const std::string &query_host,
                               const std::string &query_service);

    void startCleanupTimer();
    void stopCleanupTimer();

    /**
     * \brief Set a function to override the functionality of this class
     *
     * This is intended for testing purposes only and is NOT threadsafe. It
     * must be called before any of these classes are utilised.
     */
    static void setOverrideFunction(OverrideFunction func);

  private:
    void cleanupCache(const boost::system::error_code &ec);
};

template <typename ResolveCallback>
void DNSResolver::resolve(std::string_view       query_host,
                          std::string_view       query_service,
                          const ResolveCallback &callback)
{
    using endpointIt = boost::asio::ip::tcp::resolver::iterator;
    std::string                           host    = std::string(query_host);
    std::string                           service = std::string(query_service);
    boost::asio::ip::tcp::resolver::query query(host, service);

    if (s_override) {
        std::vector<TcpEndpoint> vec;
        auto                     ec = s_override(&vec, host, service);
        LOG_TRACE << "Returning " << vec.size()
                  << " overrriden values with ec = " << ec;
        callback(ec, vec);
        return;
    }

    {
        std::lock_guard lg(d_cacheLock);

        auto it = d_cache.find(std::make_pair(host, service));
        if (it != d_cache.end()) {
            boost::system::error_code ec;
            callback(ec, it->second);
            return;
        }
    }

    auto resolveCb =
        [this, host, service, callback](const boost::system::error_code &ec,
                                        endpointIt endpoint) {
            std::vector<TcpEndpoint> endpoints;
            endpointIt               end;
            while (endpoint != end) {
                endpoints.push_back(*endpoint);
                ++endpoint;
            }
            callback(ec, endpoints);
            setCachedResolution(host, service, std::move(endpoints));
        };

    d_resolver.async_resolve(query, resolveCb);
}

}
}

#endif
