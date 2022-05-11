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
#include <amqpprox_vhoststate.h>

#include <algorithm>
#include <iostream>
#include <map>

namespace Bloomberg {
namespace amqpprox {

VhostState::VhostState()
: d_vhosts()
, d_mutex()
{
}

VhostState::State::State()
: d_paused(false)
{
}

VhostState::State::State(const State &rhs)
: d_paused(rhs.d_paused)
{
}

bool VhostState::isPaused(const std::string &vhost)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    return d_vhosts[vhost].isPaused();
}

void VhostState::setPaused(const std::string &vhost, bool paused)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_vhosts[vhost].setPaused(paused);
}

void VhostState::print(std::ostream &os)
{
    std::map<std::string, State> sortedVhosts;

    {
        std::lock_guard<std::mutex> lg(d_mutex);
        std::copy(d_vhosts.cbegin(),
                  d_vhosts.cend(),
                  std::inserter(sortedVhosts, sortedVhosts.end()));
    }

    for (const auto &vhost : sortedVhosts) {
        auto paused = (vhost.second.isPaused() ? "PAUSED" : "UNPAUSED");
        os << vhost.first << " = " << paused << "\n";
    }
}

}
}
