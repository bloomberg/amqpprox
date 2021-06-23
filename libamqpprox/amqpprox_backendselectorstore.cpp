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
#include <amqpprox_backendselectorstore.h>

#include <amqpprox_backendselector.h>

#include <memory>
#include <string>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

// MANIPULATORS
void BackendSelectorStore::addSelector(
    std::unique_ptr<BackendSelector> selector)
{
    d_store[selector->selectorName()] = std::move(selector);
}

// ACCESSORS
BackendSelector *
BackendSelectorStore::getSelector(const std::string &name) const
{
    const auto &it = d_store.find(name);
    if (it == d_store.end()) {
        return nullptr;
    }

    return it->second.get();
}

}
}
