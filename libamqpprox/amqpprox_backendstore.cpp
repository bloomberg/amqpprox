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
#include <amqpprox_backendstore.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

BackendStore::BackendStore()
: d_backends()
{
}

int BackendStore::insert(const Backend &backend)
{
    auto namedBackend = lookup(backend.name());

    if (namedBackend) {
        return 1;
    }

    std::lock_guard<std::mutex> lg(d_mutex);
    d_backends.insert(std::make_pair(backend.name(), backend));
    auto addrKey = std::make_pair(backend.ip(), backend.port());
    return 0;
}

int BackendStore::remove(const std::string &name)
{
    auto backend = lookup(name);
    if (!backend) {
        return 1;
    }

    std::lock_guard<std::mutex> lg(d_mutex);
    auto addrKey = std::make_pair(backend->ip(), backend->port());
    d_backends.erase(name);
    return 0;
}

const Backend *BackendStore::lookup(const std::string &name) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    auto                        it = d_backends.find(name);
    if (it == d_backends.end()) {
        return nullptr;
    }
    else {
        return &(it->second);
    }
}

void BackendStore::print(std::ostream &os) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    for (const auto &backend : d_backends) {
        os << backend.second << std::endl;
    }
}

}
}
