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
#ifndef BLOOMBERG_AMQPPROX_STATCOLLECTOR
#define BLOOMBERG_AMQPPROX_STATCOLLECTOR

#include <amqpprox_connectionstats.h>
#include <amqpprox_statsnapshot.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

class BufferPool;
class CpuMonitor;
class SessionState;

/* \brief Collect statistics from Sessions for a time periodicity.
 *
 * This accumulates statistics along a number of axes such as vhost, backend
 * and source. It is designed to accumulate between those points until reset,
 * upon which time some previous values are latched and the returned values are
 * based upon the difference from the previous values with respect to each
 * axis.
 *
 * The basic workflow for dealing with this class is:
 *  1. Call `collect` with each session you want accumulated
 *  2. Call populateStats to retrieve the current statistics
 *  3. For any sessions that are no longer going to be involved in the
 *     statistics collection we call `deletedSession`
 *  4. Call `reset` to start a new collection interval
 */
class StatCollector {
  private:
    StatSnapshot d_current;
    StatSnapshot d_previous;
    CpuMonitor * d_cpuMonitor_p;  // HELD NOT OWNED
    BufferPool * d_bufferPool_p;  // HELD NOT OWNED

  public:
    // CREATORS
    StatCollector();

    // MANIPULATORS
    void reset();
    ///< Reset the current accumulation and start a new collection interval

    void collect(const SessionState &session);
    ///< Collect and accumulate statistics from the given `session`

    void deletedSession(const SessionState &session);
    ///< Handle a given `session` being deleted

    void setCpuMonitor(CpuMonitor *monitor);
    ///< Set the CPU monitor to extract CPU usage statistics from

    void setBufferPool(BufferPool *pool);
    ///< Set the buffer pool to extract statistics from

    // ACCESSORS
    void populateStats(StatSnapshot *snapshot);
    ///< Retrieve the statistics as a `snapshot` that have been accumulated
    ///< since the class was constructed, or `reset` was last called

  private:
    void populateProgramStats(ConnectionStats *programStats) const;

    void populateMap(StatSnapshot::StatsMap *      map,
                     const StatSnapshot::StatsMap &source,
                     const StatSnapshot::StatsMap &previous) const;
};

}
}

#endif
