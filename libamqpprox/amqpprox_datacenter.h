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
#ifndef BLOOMBERG_AMQPPROX_DATACENTER
#define BLOOMBERG_AMQPPROX_DATACENTER

#include <amqpprox_logging.h>

#include <string>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

class Datacenter {
  private:
    // DATA
    std::string d_datacenter;

  public:
    // MANIPULATORS
    void set(std::string datacenter);

    // ACCESSORS
    const std::string &get() const;
};

inline void Datacenter::set(std::string datacenter)
{
    LOG_DEBUG << "Set datacenter to value: " << datacenter;
    d_datacenter = std::move(datacenter);
}

inline const std::string &Datacenter::get() const
{
    return d_datacenter;
}

}
}

#endif
