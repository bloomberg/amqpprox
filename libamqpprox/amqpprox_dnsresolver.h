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
#include <boost/container_hash/hash.hpp>

#include <cstddef>
#include <functional>
#include <unordered_map>

#include <stdint.h>

namespace Bloomberg {
namespace amqpprox {

struct PairHash {
    size_t operator()(const std::pair<std::string, std::string> &obj) const
    {
        std::size_t result = 0;
        boost::hash_combine(result, obj);
        return result;
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
 *
 * The cache will only cache successful lookups.
 */
class DNSResolver {
    using TcpEndpoint = boost::asio::ip::tcp::endpoint;
    using CacheType   = std::unordered_map<std::pair<std::string, std::string>,
                                         std::vector<TcpEndpoint>,
                                         PairHash>;

  public:
    using OverrideFunction =
        std::function<boost::system::error_code(std::vector<TcpEndpoint> *,
                                                const std::string &,
                                                const std::string &)>;

  private:
    boost::asio::io_service &      d_ioService;
    boost::asio::ip::tcp::resolver d_resolver;
    boost::asio::steady_timer      d_timer;
    std::atomic<uint32_t>          d_cacheTimeout;
    std::atomic<bool>              d_cacheTimerRunning;
    std::mutex                     d_cacheLock;
    CacheType                      d_cache;

    static OverrideFunction s_override;

  public:
    /**
     * \brief Construct a resolver using io_service
     *
     * This resolver will then use the passed io_service as its event loop.
     */
    explicit DNSResolver(boost::asio::io_service &ioService);

    ~DNSResolver();

    /**
     * \brief Resolve a host string and port and invoke a functor on completion
     *
     * This is template parameterised on the `ResolveCallback` type in order to
     * match the interface of the boost resolver.
     *
     * Errors are reported through the callback's return code, and in the
     * success case the callback's endpoint vector will be updated to include a
     * possible list of entries.
     */
    template <typename ResolveCallback>
    void resolve(std::string_view       query_host,
                 std::string_view       query_service,
                 const ResolveCallback &callback);

    /**
     * \brief Set the cache timeout in milliseconds
     *
     * This will set the cache timeout to the `timeoutMs`, this doesn't take
     * effect until the next cache cleanup.
     */
    void setCacheTimeout(int timeoutMs);

    /**
     * \brief Insert a resolution into the cache
     *
     * \param query_host The host to be the key to the cache
     * \param query_service The service/port to be the key to the cache
     * \param resolution A vector of endpoints to receive as the cached result
     */
    void setCachedResolution(const std::string &        query_host,
                             const std::string &        query_service,
                             std::vector<TcpEndpoint> &&resolution);
    /**
     * \brief Clear a resolution from the cache
     *
     * \param query_host The host to be the key to the cache
     * \param query_service The service/port to be the key to the cache
     */
    void clearCachedResolution(const std::string &query_host,
                               const std::string &query_service);

    /**
     * \brief Start the cleanup timer running.
     *
     * Initially the cleanup timer is not running, this sets it running with
     * the timeout parameter supplied via a call to `setCacheTimeout`.
     */
    void startCleanupTimer();

    /**
     * \brief Stop the cleanup timer running
     *
     * This will cancel the outstanding cleanups if not already in progress.
     */
    void stopCleanupTimer();

    /**
     * \brief Set a function to override the functionality of this class
     *
     * IMPORTANT: This is intended for testing purposes only and is NOT
     * threadsafe. It must be called before any of these classes are utilised.
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

    {
        std::lock_guard lg(d_cacheLock);

        auto it = d_cache.find(std::make_pair(host, service));
        if (it != d_cache.end()) {
            boost::system::error_code ec;
            auto                      result = it->second;
            auto cb = [callback, ec, result] { callback(ec, result); };
            d_ioService.dispatch(cb);
            return;
        }
    }

    if (s_override) {
        std::vector<TcpEndpoint> vec;
        auto                     ec = s_override(&vec, host, service);
        LOG_TRACE << "Returning " << vec.size()
                  << " overriden values with ec = " << ec;
        auto cb = [callback, ec, vec] { callback(ec, vec); };
        d_ioService.dispatch(cb);

        if (!ec) {
            setCachedResolution(host, service, std::move(vec));
        }

        return;
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

            if (!ec) {
                setCachedResolution(host, service, std::move(endpoints));
            }
        };

    d_resolver.async_resolve(query, resolveCb);
}

}
}

#endif
