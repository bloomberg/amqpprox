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

/* \brief Output statistics in basic JSON notation.
 *
 * This component outputs various statistics objects used by the
 * `StatCollector` in basic machine readable JSON format onto provided ostream
 * objects.
 */
class JsonStatFormatter : public StatFormatter {
  public:
    // MANIPULATORS
    virtual void format(std::ostream &         os,
                        const ConnectionStats &connectionStats) override;

    virtual void format(std::ostream &                os,
                        const StatSnapshot::StatsMap &statsMap) override;

    virtual void format(std::ostream &      os,
                        const StatSnapshot &statSnapshot) override;

    virtual void
    format(std::ostream &                    os,
           const StatSnapshot::ProcessStats &processStats) override;

    virtual void format(std::ostream &                              os,
                        const std::vector<StatSnapshot::PoolStats> &poolStats,
                        uint64_t poolSpillover) override;
};

}
}

#endif
