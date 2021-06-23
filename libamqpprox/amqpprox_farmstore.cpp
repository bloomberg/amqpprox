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
#include <amqpprox_farmstore.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

// CREATORS
FarmStore::FarmStore()
: d_farms()
{
}

// MANIPULATORS
void FarmStore::addFarm(std::unique_ptr<Farm> farm)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    if (d_farms.count(farm->name()) > 0) {
        d_farms[farm->name()] = std::move(farm);
    }
    else {
        d_farms.emplace(std::make_pair(farm->name(), std::move(farm)));
    }
}

void FarmStore::removeFarmByName(const std::string &farmName)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_farms.erase(farmName);
}

void FarmStore::repartitionAll()
{
    std::lock_guard<std::mutex> lg(d_mutex);
    for (auto &farm : d_farms) {
        farm.second->repartition();
    }
}

// ACCESSORS
Farm &FarmStore::getFarmByName(const std::string &name) const
{
    std::lock_guard<std::mutex> lg(d_mutex);

    const auto &farm = d_farms.find(name);
    if (farm == d_farms.end()) {
        throw std::runtime_error("No such farm");
    }

    return *(farm->second);
}

void FarmStore::print(std::ostream &os) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    for (const auto &farm : d_farms) {
        os << *(farm.second) << "\n";
    }
}

}
}
