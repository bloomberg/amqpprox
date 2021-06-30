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
#ifndef BLOOMBERG_AMQPPROX_JSONSTATFORMATTER
#define BLOOMBERG_AMQPPROX_JSONSTATFORMATTER

#include <amqpprox_statformatter.h>
#include <amqpprox_statsnapshot.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Output statistics in basic JSON notation.
 *
 * This component outputs various statistics objects used by the
 * `StatCollector` in basic machine readable JSON format onto provided ostream
 * objects.
 */
class JsonStatFormatter : public StatFormatter {
  public:
    // MANIPULATORS
    /**
     * \brief output the connection stats into the output stream in a JSON
     * format.
     *
     * \param os the output stream
     *
     * \param connectionStats reference to the ConnectionStats
     */
    virtual void format(std::ostream &         os,
                        const ConnectionStats &connectionStats) override;

    /**
     * \brief output the `StatSnapshot::StatsMap` into the output stream in a
     * JSON format.
     *
     * \param os the output stream
     *
     * \param statsMap reference to the StatsMap
     */
    virtual void format(std::ostream &                os,
                        const StatSnapshot::StatsMap &statsMap) override;
    /**
     * \brief output the `StatSnapshot` into the output stream in a
     * JSON format.
     *
     * \param os the output stream
     *
     * \param statSnapshot reference to the StatSnapshot
     */
    virtual void format(std::ostream &      os,
                        const StatSnapshot &statSnapshot) override;

    /**
     * \brief output the `StatSnapshot::ProcessStats` into the output stream in
     * a JSON format.
     *
     * \param os the output stream
     *
     * \param processStats reference to the ProcessStats
     */
    virtual void
    format(std::ostream &                    os,
           const StatSnapshot::ProcessStats &processStats) override;
    /**
     * \brief output the `StatSnapshot::PoolStats` into the output stream in
     * a JSON format.
     *
     * \param os the output stream
     *
     * \param poolStats reference to the vector of PoolStats
     *
     * \param poolSpillover the amount of poolSpillover
     */
    virtual void format(std::ostream &                              os,
                        const std::vector<StatSnapshot::PoolStats> &poolStats,
                        uint64_t poolSpillover) override;
};

}
}

#endif
