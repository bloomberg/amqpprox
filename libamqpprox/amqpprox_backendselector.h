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
#ifndef BLOOMBERG_AMQPPROX_BACKENDSELECTOR
#define BLOOMBERG_AMQPPROX_BACKENDSELECTOR

#include <cstdint>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class Backend;
class BackendSet;

class BackendSelector {
  public:
    // CREATORS
    virtual ~BackendSelector() = default;

    // ACCESSORS
    virtual const Backend *select(BackendSet *                 backendSet,
                                  const std::vector<uint64_t> &markers,
                                  uint64_t retryCount) const = 0;
    ///< Select a `Backend` instance from the specified `BackendSet` that
    ///< should be connected to based on the specified `markers` and
    ///< `retryCount`. When a `Backend` is returned, the relevant
    ///< `Partition` will be marked.
    ///< If no `Backend` is suitable based on the specified values, a null
    ///< pointer will be returned.

    // ACCESSORS
    virtual const std::string &selectorName() const = 0;
    ///< Return the name of this `BackendSelector`. This name is used to
    ///< attach this selector to a given `Farm`.
};

}
}

#endif
