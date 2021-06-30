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
#ifndef BLOOMBERG_AMQPPROX_STATFORMATTER
#define BLOOMBERG_AMQPPROX_STATFORMATTER

#include <amqpprox_statsnapshot.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Provides a pure virtual interface to output statistics in various
 * format
 *
 * The interface provides different methods to output various statistics
 * objects used by the `StatCollector` in implemented format onto provided
 * ostream objects.
 */
class StatFormatter {
  public:
    // CREATORS
    virtual ~StatFormatter() = default;

    // MANIPULATORS
    /**
     * \brief output the connection stats into the output stream in the
     * implemented format.
     * \param os the output stream
     * \param connectionStats const reference to the `ConnectionStats`
     */
    virtual void format(std::ostream &         os,
                        const ConnectionStats &connectionStats) = 0;

    /**
     * \brief output the `StatSnapshot::StatsMap` into the output stream in the
     * implemented format.
     * \param os the output stream
     * \param statsMap const reference to the `StatSnapshot::StatsMap`
     */
    virtual void format(std::ostream &                os,
                        const StatSnapshot::StatsMap &statsMap) = 0;

    /**
     * \brief output the `StatSnapshot` into the output stream in the
     * implemented format.
     * \param os the output stream
     * \param statSnapshot const reference to the `StatSnapshot`
     */
    virtual void format(std::ostream &      os,
                        const StatSnapshot &statSnapshot) = 0;

    /**
     * \brief output the `StatSnapshot::ProcessStats` into the output stream in
     * the implemented format.
     * \param os the output stream
     * \param processStats const reference to the `StatSnapshot::ProcessStats`
     */
    virtual void format(std::ostream &                    os,
                        const StatSnapshot::ProcessStats &processStats) = 0;

    /**
     * \brief output the `StatSnapshot::PoolStats` into the output stream in
     * the implemented format.
     * \param os the output stream
     * \param poolStats const reference to the vector of
     * `StatSnapshot::PoolStats`
     * \param poolSpillover the amount of poolSpillover
     */
    virtual void format(std::ostream &                              os,
                        const std::vector<StatSnapshot::PoolStats> &poolStats,
                        uint64_t poolSpillover) = 0;
};

}
}

#endif
