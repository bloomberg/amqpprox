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
#ifndef BLOOMBERG_AMQPPROX_BACKENDSTORE
#define BLOOMBERG_AMQPPROX_BACKENDSTORE

#include <amqpprox_backend.h>

#include <iosfwd>
#include <map>
#include <mutex>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

class BackendStore {
    std::unordered_map<std::string, Backend>               d_backends;
    std::map<std::pair<std::string, int>, const Backend *> d_backendAddresses;
    mutable std::mutex                                     d_mutex;

  public:
    BackendStore();
    int insert(const Backend &backend);
    int remove(const std::string &name);

    const Backend *lookup(const std::string &ip, int port) const;
    const Backend *lookup(const std::string &name) const;
    void           print(std::ostream &os) const;
};

}
}

#endif
