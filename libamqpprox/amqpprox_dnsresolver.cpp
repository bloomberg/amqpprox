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

#include <amqpprox_dnsresolver.h>

#include <amqpprox_logging.h>

#include <boost/system/error_code.hpp>

#include <chrono>
#include <mutex>

namespace Bloomberg {
namespace amqpprox {

DNSResolver::DNSResolver(boost::asio::io_service &ioService)
: d_ioService(ioService)
, d_resolver(d_ioService)
, d_timer(d_ioService)
, d_cacheTimeout(1000)
, d_cacheTimerRunning(false)
{
}

DNSResolver::~DNSResolver()
{
    d_resolver.cancel();
    stopCleanupTimer();
}

void DNSResolver::setCacheTimeout(int timeoutMs)
{
    d_cacheTimeout = timeoutMs;
}

void DNSResolver::startCleanupTimer()
{
    bool previous       = d_cacheTimerRunning;
    d_cacheTimerRunning = true;
    if (!previous) {
        d_timer.expires_after(std::chrono::milliseconds(d_cacheTimeout));
        d_timer.async_wait(
            [this](const boost::system::error_code &ec) { cleanupCache(ec); });
    }
}

void DNSResolver::stopCleanupTimer()
{
    d_cacheTimerRunning = false;
    d_timer.cancel();
}

void DNSResolver::cleanupCache(const boost::system::error_code &ec)
{
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            LOG_ERROR << "DNSResolver cache clean up failed with: " << ec;
        }
        return;
    }
    if (d_cacheTimerRunning) {
        {
            CacheType       empty;
            std::lock_guard lg(d_cacheLock);
            d_cache.swap(empty);
        }
        d_timer.expires_after(std::chrono::milliseconds(d_cacheTimeout));
        d_timer.async_wait(
            [this](const boost::system::error_code &ec) { cleanupCache(ec); });
    }
}

}
}
