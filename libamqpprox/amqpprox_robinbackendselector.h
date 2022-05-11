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
#ifndef BLOOMBERG_AMQPPROX_ROBINBACKENDSELECTOR
#define BLOOMBERG_AMQPPROX_ROBINBACKENDSELECTOR

#include <amqpprox_backendselector.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class Backend;
class BackendSet;

/**
 * \brief Selects the available Backend instance from the set using
 * round-robin algorithm, implements the BackendSelector interface
 */
class RobinBackendSelector : public BackendSelector {
  public:
    // CREATORS
    virtual ~RobinBackendSelector() override = default;

    // ACCESSORS
    virtual const Backend *select(BackendSet                  *backendSet,
                                  const std::vector<uint64_t> &markers,
                                  uint64_t retryCount) const override;

    // ACCESSORS
    /**
     * \return the name of this `BackendSelector`. This name is used to attach
     * this selector to a given `Farm`.
     */
    virtual const std::string &selectorName() const override;
};

}
}

#endif
