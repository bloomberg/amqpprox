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
#include <amqpprox_dnshostnamemapper.h>

#include <amqpprox_logging.h>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

namespace Bloomberg {
namespace amqpprox {

namespace {

// TODO: we could consider making this configurable.  For our usage we
// can just hardcode it for now though
const size_t CACHE_SIZE_WARN_LIMIT = 50000;

}

DNSHostnameMapper::DNSHostnameMapper()
: d_hostnameMap()
{
}

void DNSHostnameMapper::prime(
    boost::asio::io_context                              &ioContext,
    std::initializer_list<boost::asio::ip::tcp::endpoint> endpoints)
{
    boost::asio::ip::tcp::resolver resolver(ioContext);
    for (const auto &endpoint : endpoints) {
        auto address = boost::lexical_cast<std::string>(endpoint.address());
        boost::upgrade_lock<boost::shared_mutex> lock(d_lg);
        auto found = d_hostnameMap.find(address);
        if (found != d_hostnameMap.end()) {
            continue;
        }
        boost::system::error_code ec;
        auto                      destination = resolver.resolve(endpoint, ec);
        if (ec) {
            LOG_ERROR << "Failed to resolve hostname for "
                      << endpoint.address() << " error code: " << ec;
            continue;
        }

        if (d_hostnameMap.size() + 1 > CACHE_SIZE_WARN_LIMIT) {
            LOG_WARN << "The size of the hostname cache is larger than the "
                     << "warning threshold. (" << d_hostnameMap.size() + 1
                     << " > " << CACHE_SIZE_WARN_LIMIT << ")";
        }

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        d_hostnameMap.insert({address, destination->host_name()});
    }
}

std::string DNSHostnameMapper::mapToHostname(
    const boost::asio::ip::tcp::endpoint &endpoint) const
{
    boost::shared_lock<boost::shared_mutex> lock(d_lg);
    auto address = boost::lexical_cast<std::string>(endpoint.address());
    if (address == "0.0.0.0") {
        return address;
    }

    auto found = d_hostnameMap.find(address);
    if (found == d_hostnameMap.end()) {
        LOG_ERROR << "Failed to get address from hostname cache";
        return boost::lexical_cast<std::string>(address);
    }

    return found->second;
}

}
}
