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
#ifndef BLOOMBERG_AMQPPROX_RESOURCEMAPPER
#define BLOOMBERG_AMQPPROX_RESOURCEMAPPER

#include <iosfwd>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

class SessionState;

class ResourceMapper {
    using Resource = std::pair<bool, std::string>;
    std::unordered_map<std::string, Resource> d_mappings;
    mutable std::mutex                        d_mutex;

  public:
    ResourceMapper();

    // MANIPULATORS
    void mapVhostToFarm(const std::string &vhost, const std::string &farmName);

    void mapVhostToBackend(const std::string &vhost,
                           const std::string &backendName);

    void unmapVhost(const std::string &vhost);

    // ACCESSORS
    bool getResourceMap(bool *              isFarm,
                        std::string *       resourceName,
                        const SessionState &state) const;

    void print(std::ostream &os) const;
};

}
}

#endif
