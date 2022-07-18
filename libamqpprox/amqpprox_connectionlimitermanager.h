/*
** Copyright 2022 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONLIMITERMANAGER
#define BLOOMBERG_AMQPPROX_CONNECTIONLIMITERMANAGER

#include <amqpprox_connectionlimiterinterface.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Manages different connection limiters for different vhosts and also
 * provides default connection rate limiter for all the vhosts, if configured.
 * At any point only one type of limiter will be applied for particular vhost.
 * But any vhost can have multiple types limiters. If the limiter is applied in
 * alarm only mode, then the ConnectionLimiterManager will only emit log at
 * warning level with AMQPPROX_CONNECTION_LIMIT as a substring and the
 * relevant limiter details without notifying the caller upon connection limit
 * violation.
 *
 * \note All the data members in this class should be modified in synchronous
 * way. The data members will be used by a thread issung control commands and
 * primary Server thread. So all the required function should access the data
 * members using shared mutex lock to avoid race conditions.
 */
class ConnectionLimiterManager {
  public:
    typedef std::unordered_map<
        std::string,
        std::pair<bool, std::shared_ptr<ConnectionLimiterInterface>>>
        ConnectionLimiters;

  private:
    // Represents connectionRateLimiters per vhost with boolean to specify
    // whether the limiter is applying default limit or custom limit for
    // specific vhost. True bool value represents custom connection rate limit
    // for the vhost
    ConnectionLimiters d_connectionRateLimitersPerVhost;
    ConnectionLimiters d_alarmOnlyConnectionRateLimitersPerVhost;
    ConnectionLimiters d_totalConnectionLimitersPerVhost;
    ConnectionLimiters d_alarmOnlyTotalConnectionLimitersPerVhost;

    std::optional<uint32_t> d_defaultConnectionRateLimit;
    std::optional<uint32_t> d_defaultAlarmOnlyConnectionRateLimit;
    std::optional<uint32_t> d_defaultTotalConnectionLimit;
    std::optional<uint32_t> d_defaultAlarmOnlyTotalConnectionLimit;
    mutable std::mutex      d_mutex;

  public:
    // CREATORS
    ConnectionLimiterManager();

    // MANIPULATORS
    /**
     * \brief Add new connection rate limiter or modify existing connection
     * rate limiter for specified vhost
     * \param vhostName vhost name
     * \param numberOfConnections limit number of connections per second
     * \return the added connection rate limiter
     */
    std::shared_ptr<ConnectionLimiterInterface>
    addConnectionRateLimiter(const std::string &vhostName,
                             uint32_t           numberOfConnections);

    /**
     * \brief Add new connection rate limiter or modify existing connection
     * rate limiter for specified vhost in alarm only mode. The limiter will
     * only emit log at warning level with AMQPPROX_CONNECTION_LIMIT as a
     * substring and the relevant limiter details, instead of limiting actual
     * connection
     * \param vhostName vhost name
     * \param numberOfConnections limit number of connections per second
     * \return the added connection rate limiter
     */
    std::shared_ptr<ConnectionLimiterInterface>
    addAlarmOnlyConnectionRateLimiter(const std::string &vhostName,
                                      uint32_t           numberOfConnections);

    /**
     * \brief Add new total connection limiter or modify existing total
     * connection limiter for specified vhost
     * \param vhostName vhost name
     * \param numberOfConnections limit number of total connections
     * \return the added total connection limiter
     */
    std::shared_ptr<ConnectionLimiterInterface>
    addTotalConnectionLimiter(const std::string &vhostName,
                              uint32_t           numberOfConnections);

    /**
     * \brief Add new total connection limiter or modify existing total
     * connection limiter for specified vhost in alarm only mode. The limiter
     * will only emit log at warning level with AMQPPROX_CONNECTION_LIMIT as a
     * substring and the relevant limiter details, instead of limiting actual
     * connection
     * \param vhostName vhost name
     * \param numberOfConnections limit number of total connections
     * \return the added total connection limiter
     */
    std::shared_ptr<ConnectionLimiterInterface>
    addAlarmOnlyTotalConnectionLimiter(const std::string &vhostName,
                                       uint32_t           numberOfConnections);

    /**
     * \brief Set default connection rate limit for all connecting vhosts
     * \param defaultConnectionRateLimit default connection rate (allowed
     * connections per second)
     */
    void setDefaultConnectionRateLimit(uint32_t defaultConnectionRateLimit);

    /**
     * \brief Set default connection rate limit for all connecting vhosts in
     * alarm only mode. The limiter will only emit log at warning level with
     * AMQPPROX_CONNECTION_LIMIT as a substring and the relevant limiter
     * details, instead of limiting actual connection
     * \param defaultConnectionRateLimit default connection rate (allowed
     * connections per second)
     */
    void setAlarmOnlyDefaultConnectionRateLimit(
        uint32_t defaultConnectionRateLimit);

    /**
     * \brief Set default total connection limit for all connecting vhosts
     * \param defaultTotalConnectionLimit default total connection limit
     * (allowed total connections)
     */
    void setDefaultTotalConnectionLimit(uint32_t defaultTotalConnectionLimit);

    /**
     * \brief Set default total connection limit for all connecting vhosts in
     * alarm only mode. The limiter will only emit log at warning level with
     * AMQPPROX_CONNECTION_LIMIT as a substring and the relevant limiter
     * details, instead of limiting actual connection
     * \param defaultTotalConnectionLimit default total connection limit
     * (allowed total connections)
     */
    void setAlarmOnlyDefaultTotalConnectionLimit(
        uint32_t defaultTotalConnectionLimit);

    /**
     * \brief Remove specific connection rate limiter for specified vhost
     * \param vhostName vhost name
     */
    void removeConnectionRateLimiter(const std::string &vhostName);

    /**
     * \brief Remove specific alarm only connection rate limiter for specified
     * vhost
     * \param vhostName vhost name
     */
    void removeAlarmOnlyConnectionRateLimiter(const std::string &vhostName);

    /**
     * \brief Remove specific total connection limiter for specified vhost
     * \param vhostName vhost name
     */
    void removeTotalConnectionLimiter(const std::string &vhostName);

    /**
     * \brief Remove specific alarm only total connection limiter for specified
     * vhost
     * \param vhostName vhost name
     */
    void removeAlarmOnlyTotalConnectionLimiter(const std::string &vhostName);

    /**
     * \brief Remove default connection rate limit for all the connecting
     * vhosts
     */
    void removeDefaultConnectionRateLimit();

    /**
     * \brief Remove default alarm only connection rate limit for all the
     * connecting vhosts
     */
    void removeAlarmOnlyDefaultConnectionRateLimit();

    /**
     * \brief Remove default total connection limit for all the connecting
     * vhosts
     */
    void removeDefaultTotalConnectionLimit();

    /**
     * \brief Remove default alarm only total connection rate limit for all the
     * connecting vhosts
     */
    void removeAlarmOnlyDefaultTotalConnectionLimit();

    /**
     * \brief Decide whether the current connection request should be allowed
     * or not based on configured different limiters for the specified vhost
     * \param vhostName vhost name
     */
    bool allowNewConnectionForVhost(const std::string &vhostName);

    /**
     * \brief Called when an aquired connection is closed
     */
    void connectionClosed(const std::string &vhostName);

    // ACCESSORS
    /**
     * \brief Get particular connection rate limiter based on specified vhost
     * \param vhostName vhost name
     */
    std::shared_ptr<ConnectionLimiterInterface>
    getConnectionRateLimiter(const std::string &vhostName) const;

    /**
     * \brief Get particular alarm only connection rate limiter based on
     * specified vhost
     * \param vhostName vhost name
     */
    std::shared_ptr<ConnectionLimiterInterface>
    getAlarmOnlyConnectionRateLimiter(const std::string &vhostName) const;

    /**
     * \brief Get particular total connection limiter based on specified vhost
     * \param vhostName vhost name
     */
    std::shared_ptr<ConnectionLimiterInterface>
    getTotalConnectionLimiter(const std::string &vhostName) const;

    /**
     * \brief Get particular alarm only total connection limiter based on
     * specified vhost
     * \param vhostName vhost name
     */
    std::shared_ptr<ConnectionLimiterInterface>
    getAlarmOnlyTotalConnectionLimiter(const std::string &vhostName) const;

    /**
     * \brief Get default connection rate limit (allowed connections per
     * second) for all the connecting vhosts
     */
    std::optional<uint32_t> getDefaultConnectionRateLimit() const;

    /**
     * \brief Get alarm only default connection rate limit (allowed connections
     * per second) for all the connecting vhosts
     */
    std::optional<uint32_t> getAlarmOnlyDefaultConnectionRateLimit() const;

    /**
     * \brief Get default total connection limit (allowed total connections)
     * for all the connecting vhosts
     */
    std::optional<uint32_t> getDefaultTotalConnectionLimit() const;

    /**
     * \brief Get alarm only default total connection limit (allowed total
     * connections) for all the connecting vhosts
     */
    std::optional<uint32_t> getAlarmOnlyDefaultTotalConnectionLimit() const;
};

}
}

#endif
