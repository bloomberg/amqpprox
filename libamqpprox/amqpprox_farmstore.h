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
#ifndef BLOOMBERG_AMQPPROX_FARMSTORE
#define BLOOMBERG_AMQPPROX_FARMSTORE

#include <amqpprox_farm.h>

#include <iosfwd>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

/**
 *  \brief Provides a collection of farms which reference selection policies
 * and current members.
 */
class FarmStore {
  private:
    // PRIVATE TYPES
    using FarmMap = std::unordered_map<std::string, std::unique_ptr<Farm>>;

    // DATA
    FarmMap            d_farms;
    mutable std::mutex d_mutex;

  public:
    // CREATORS
    FarmStore();

    // MANIPULATORS
    /**
     * \brief Add or override a farm by name
     */
    void addFarm(std::unique_ptr<Farm> farm);

    /**
     * \brief Remove a farm by its name
     * \param farmName the name of the farm to remove
     */
    void removeFarmByName(const std::string &farmName);

    /**
     * \brief Repartition all of the farms stored
     */
    void repartitionAll();

    // ACCESSORS
    /**
     * \brief Return a modifiable reference to the farm associated with this
     * name
     * \param name of farm to retrieve
     * \returns a reference to the Farm named
     */
    Farm &getFarmByName(const std::string &name) const;

    /**
     * \brief Print all of the farms in the store
     */
    void print(std::ostream &os) const;
};

}
}

#endif
