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
#include <amqpprox_resourcemapper.h>

#include <amqpprox_sessionstate.h>

#include <map>

namespace Bloomberg {
namespace amqpprox {

ResourceMapper::ResourceMapper()
: d_mappings()
, d_mutex()
{
}

void ResourceMapper::mapVhostToFarm(const std::string &vhost,
                                    const std::string &farmName)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_mappings[vhost] = std::make_pair(true, farmName);
}

void ResourceMapper::mapVhostToBackend(const std::string &vhost,
                                       const std::string &backendName)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_mappings[vhost] = std::make_pair(false, backendName);
}

void ResourceMapper::unmapVhost(const std::string &vhost)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_mappings.erase(vhost);
}

// ACCESSORS
bool ResourceMapper::getResourceMap(bool *              isFarm,
                                    std::string *       resourceName,
                                    const SessionState &state) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    auto                        it = d_mappings.find(state.getVirtualHost());
    if (it != d_mappings.end()) {
        auto val      = it->second;
        *isFarm       = val.first;
        *resourceName = val.second;
        return true;
    }
    return false;
}

void ResourceMapper::print(std::ostream &os) const
{
    std::map<std::string, Resource> sortedMappings;

    {
        std::lock_guard<std::mutex> lg(d_mutex);
        std::copy(d_mappings.cbegin(),
                  d_mappings.cend(),
                  std::inserter(sortedMappings, sortedMappings.end()));
    }

    for (const auto &resource : sortedMappings) {
        os << "\"" << resource.first << "\" => ";
        if (resource.second.first) {
            os << "Farm:";
        }
        else {
            os << "Backend:";
        }
        os << resource.second.second << "\n";
    }
}

}
}
