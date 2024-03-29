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

#include <amqpprox_connectionlimitermanager.h>

#include <amqpprox_connectionlimiterinterface.h>
#include <amqpprox_fixedwindowconnectionratelimiter.h>
#include <amqpprox_logging.h>

#include <memory>
#include <optional>
#include <string>

namespace Bloomberg {
namespace amqpprox {

namespace {
void maybePopulateDefaultLimiters(
    const std::string                            &vhostName,
    std::optional<uint32_t>                       defaultLimit,
    ConnectionLimiterManager::ConnectionLimiters &limitersPerVhost)
{
    if (limitersPerVhost.find(vhostName) == limitersPerVhost.end()) {
        if (defaultLimit) {
            limitersPerVhost[vhostName] = {
                false,
                std::make_shared<FixedWindowConnectionRateLimiter>(
                    *defaultLimit)};
        }
    }
}
}

ConnectionLimiterManager::ConnectionLimiterManager()
: d_connectionRateLimitersPerVhost()
, d_alarmOnlyConnectionRateLimitersPerVhost()
, d_defaultConnectionRateLimit()
, d_defaultAlarmOnlyConnectionRateLimit()
, d_mutex()
{
}

std::shared_ptr<ConnectionLimiterInterface>
ConnectionLimiterManager::addConnectionRateLimiter(
    const std::string &vhostName,
    uint32_t           numberOfConnections)
{
    std::shared_ptr<FixedWindowConnectionRateLimiter> connectionRateLimiter =
        std::make_shared<FixedWindowConnectionRateLimiter>(
            numberOfConnections);

    std::lock_guard<std::mutex> lg(d_mutex);
    d_connectionRateLimitersPerVhost[vhostName] = {true,
                                                   connectionRateLimiter};
    return connectionRateLimiter;
}

std::shared_ptr<ConnectionLimiterInterface>
ConnectionLimiterManager::addAlarmOnlyConnectionRateLimiter(
    const std::string &vhostName,
    uint32_t           numberOfConnections)
{
    std::shared_ptr<FixedWindowConnectionRateLimiter>
        alarmOnlyConnectionRateLimiter =
            std::make_shared<FixedWindowConnectionRateLimiter>(
                numberOfConnections);

    std::lock_guard<std::mutex> lg(d_mutex);
    d_alarmOnlyConnectionRateLimitersPerVhost[vhostName] = {
        true, alarmOnlyConnectionRateLimiter};
    return alarmOnlyConnectionRateLimiter;
}

void ConnectionLimiterManager::setDefaultConnectionRateLimit(
    uint32_t defaultConnectionRateLimit)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultConnectionRateLimit = defaultConnectionRateLimit;
    // To update new default connection rate limit for all the vhosts,
    // by removing old already set default connection rate limiters
    for (auto &limiter : d_connectionRateLimitersPerVhost) {
        if (!limiter.second.first) {
            limiter.second.second =
                std::make_shared<FixedWindowConnectionRateLimiter>(
                    *d_defaultConnectionRateLimit);
        }
    }
}

void ConnectionLimiterManager::setAlarmOnlyDefaultConnectionRateLimit(
    uint32_t defaultConnectionRateLimit)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultAlarmOnlyConnectionRateLimit = defaultConnectionRateLimit;
    // To update new default alarm only connection rate limit for all the
    // vhosts, by removing old already set default alarm only connection rate
    // limiters
    for (auto &limiter : d_alarmOnlyConnectionRateLimitersPerVhost) {
        if (!limiter.second.first) {
            limiter.second.second =
                std::make_shared<FixedWindowConnectionRateLimiter>(
                    *d_defaultAlarmOnlyConnectionRateLimit);
        }
    }
}

void ConnectionLimiterManager::removeConnectionRateLimiter(
    const std::string &vhostName)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    if (d_defaultConnectionRateLimit) {
        d_connectionRateLimitersPerVhost[vhostName] = {
            false,
            std::make_shared<FixedWindowConnectionRateLimiter>(
                *d_defaultConnectionRateLimit)};
    }
    else {
        d_connectionRateLimitersPerVhost.erase(vhostName);
    }
}

void ConnectionLimiterManager::removeAlarmOnlyConnectionRateLimiter(
    const std::string &vhostName)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    if (d_defaultAlarmOnlyConnectionRateLimit) {
        d_alarmOnlyConnectionRateLimitersPerVhost[vhostName] = {
            false,
            std::make_shared<FixedWindowConnectionRateLimiter>(
                *d_defaultAlarmOnlyConnectionRateLimit)};
    }
    else {
        d_alarmOnlyConnectionRateLimitersPerVhost.erase(vhostName);
    }
}

void ConnectionLimiterManager::removeDefaultConnectionRateLimit()
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultConnectionRateLimit.reset();
    for (auto it = d_connectionRateLimitersPerVhost.cbegin();
         it != d_connectionRateLimitersPerVhost.cend();) {
        if (!it->second.first) {
            it = d_connectionRateLimitersPerVhost.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ConnectionLimiterManager::removeAlarmOnlyDefaultConnectionRateLimit()
{
    std::lock_guard<std::mutex> lg(d_mutex);

    d_defaultAlarmOnlyConnectionRateLimit.reset();
    for (auto it = d_alarmOnlyConnectionRateLimitersPerVhost.cbegin();
         it != d_alarmOnlyConnectionRateLimitersPerVhost.cend();) {
        if (!it->second.first) {
            it = d_alarmOnlyConnectionRateLimitersPerVhost.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool ConnectionLimiterManager::allowNewConnectionForVhost(
    const std::string &vhostName)
{
    std::lock_guard<std::mutex> lg(d_mutex);

    maybePopulateDefaultLimiters(vhostName,
                                 d_defaultAlarmOnlyConnectionRateLimit,
                                 d_alarmOnlyConnectionRateLimitersPerVhost);
    maybePopulateDefaultLimiters(vhostName,
                                 d_defaultConnectionRateLimit,
                                 d_connectionRateLimitersPerVhost);

    auto alarmLimiter =
        d_alarmOnlyConnectionRateLimitersPerVhost.find(vhostName);
    if (alarmLimiter != d_alarmOnlyConnectionRateLimitersPerVhost.end()) {
        if (!(alarmLimiter->second.second->allowNewConnection())) {
            if (alarmLimiter->second.first) {
                LOG_WARN << "AMQPPROX_CONNECTION_LIMIT: The connection "
                            "request for "
                         << vhostName << " should be limited by "
                         << alarmLimiter->second.second->toString();
            }
            else {
                LOG_WARN << "AMQPPROX_CONNECTION_LIMIT: The connection "
                            "request for "
                         << vhostName << " should be limited by default "
                         << alarmLimiter->second.second->toString();
            }
        }
    }

    auto limiter = d_connectionRateLimitersPerVhost.find(vhostName);
    if (limiter != d_connectionRateLimitersPerVhost.end()) {
        if (!(limiter->second.second->allowNewConnection())) {
            if (limiter->second.first) {
                LOG_DEBUG
                    << "AMQPPROX_CONNECTION_LIMIT: The connection request for "
                    << vhostName << " is limited by "
                    << limiter->second.second->toString();
                return false;
            }
            else {
                LOG_DEBUG
                    << "AMQPPROX_CONNECTION_LIMIT: The connection request for "
                    << vhostName << " is limited by default "
                    << limiter->second.second->toString();
                return false;
            }
        }
    }

    return true;
}

std::shared_ptr<ConnectionLimiterInterface>
ConnectionLimiterManager::getConnectionRateLimiter(
    const std::string &vhostName) const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    auto limiter = d_connectionRateLimitersPerVhost.find(vhostName);
    if (limiter != d_connectionRateLimitersPerVhost.end()) {
        return limiter->second.second;
    }
    return nullptr;
}

std::shared_ptr<ConnectionLimiterInterface>
ConnectionLimiterManager::getAlarmOnlyConnectionRateLimiter(
    const std::string &vhostName) const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    auto alarmLimiter =
        d_alarmOnlyConnectionRateLimitersPerVhost.find(vhostName);
    if (alarmLimiter != d_alarmOnlyConnectionRateLimitersPerVhost.end()) {
        return alarmLimiter->second.second;
    }
    return nullptr;
}

std::optional<uint32_t>
ConnectionLimiterManager::getDefaultConnectionRateLimit() const
{
    return d_defaultConnectionRateLimit;
}

std::optional<uint32_t>
ConnectionLimiterManager::getAlarmOnlyDefaultConnectionRateLimit() const
{
    return d_defaultAlarmOnlyConnectionRateLimit;
}

}
}
