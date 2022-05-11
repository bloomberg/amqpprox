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

/**
 * \brief Collect statistics from Sessions for a time periodicity.
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
    CpuMonitor  *d_cpuMonitor_p;  // HELD NOT OWNED
    BufferPool  *d_bufferPool_p;  // HELD NOT OWNED

  public:
    // CREATORS
    StatCollector();

    // MANIPULATORS
    /**
     * \brief Reset the current accumulation and start a new collection
     * interval
     */
    void reset();

    /**
     * \brief Collect and accumulate statistics from the given `session`
     * \param session `SessionState` object, contains a counter of total bytes,
     * packets and frames for each direction
     */
    void collect(const SessionState &session);

    /**
     * \brief Delete the accumulated metrics for deleted specified session
     * \param session `SessionState` object, whose accumulated metrics will be
     * removed from maintined collection
     */
    void deletedSession(const SessionState &session);

    /**
     * \brief Set the CPU monitor to extract CPU usage statistics from
     * \param monitor pointer to `CpuMonitor`
     */
    void setCpuMonitor(CpuMonitor *monitor);

    /**
     * \brief Set the buffer pool to extract statistics from
     * \param pool pointer to `BuffrePool`
     */
    void setBufferPool(BufferPool *pool);

    // ACCESSORS
    /**
     * \brief Retrieve the statistics as a `snapshot` that have been
     * accumulated since the class was constructed, or `reset` was last called
     * \param snapshot pointere to `StatSnapshot`
     */
    void populateStats(StatSnapshot *snapshot);

  private:
    void populateProgramStats(ConnectionStats *programStats) const;

    void populateMap(StatSnapshot::StatsMap       *map,
                     const StatSnapshot::StatsMap &source,
                     const StatSnapshot::StatsMap &previous) const;
};

}
}

#endif
