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
#ifndef BLOOMBERG_AMQPPROX_VHOSTSTATE
#define BLOOMBERG_AMQPPROX_VHOSTSTATE

#include <mutex>
#include <string>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Stores state related to vhosts
 *
 * This component stores state related to vhosts which has to be kept the same
 * across all connections and even while there are no connections for a
 * particular vhost.
 *
 * This component is fully-threadsafe in its methods.
 */
class VhostState {
    class State {
        bool d_paused;

      public:
        State();
        State(const State& rhs);

        inline bool isPaused() const { return d_paused; }

        inline void setPaused(bool paused) { d_paused = paused; }
    };

    std::unordered_map<std::string, State> d_vhosts;
    std::mutex                             d_mutex;

  public:
    VhostState();

    /**
     * \brief Retrieve the paused state for a vhost
     *
     * \param vhost The vhost name to retrieve the state for
     * \return The pause state as a boolean where true is paused
     */
    bool isPaused(const std::string &vhost);

    /** 
     * \brief Set the specified vhost to the given paused state
     *
     * \param vhost The vhost to be manipulated
     * \param paused The paused state of the vhost
     */
    void setPaused(const std::string &vhost, bool paused);

    /**
     * \brief Print the mappings currently held to the provide ostream
     *
     * \param os The ostream to print to
     */
    void print(std::ostream& os);
};

}
}

#endif
