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
#ifndef BLOOMBERG_AMQPPROX_CPUMONITOR
#define BLOOMBERG_AMQPPROX_CPUMONITOR

#include <cstdint>
#include <cstring>
#include <mutex>
#include <tuple>

#include <boost/timer/timer.hpp>

#include <sys/time.h>

namespace Bloomberg {
namespace amqpprox {

class Control;
class Server;

/**
 * \brief Class for monitoring CPU usage
 */
class CpuMonitor {
    double                  d_currentCpuUser;
    double                  d_currentCpuSystem;
    std::size_t             d_currentMaxRssKB;
    uint64_t                d_samples;
    boost::timer::cpu_timer d_cpuTimer;
    mutable std::mutex      d_mutex;

  public:
    // TYPES
    /**
     * \brief Tuple encoding the ratio of user cpu time versus wall time for
     * the first value, and system cpu time versus wall time for the second.
     */
    using UserSystemUsage = std::tuple<double, double>;

    // CREATORS
    CpuMonitor();

    // MANIPULATORS
    /**
     * \brief Sample the process statistics
     */
    bool clock(Control *unused1, Server *unused2);

    // ACCESSORS
    /**
     * \return Tuple of ratios of cpu clock versus wallclock, first
     * element is the user time, and second is the system time.
     */
    UserSystemUsage currentCpu() const;

    /**
     * \return RSS in KiloBytes
     */
    std::size_t currentRssKB() const;

    /**
     * \return True iff enough samples have been collected for the stats to be
     * valid.
     */
    bool valid() const;

    // STATIC ACCESSORS
    /**
     * \return Time difference to clocking cycles in milliseconds
     */
    static int intervalMs();
};

}
}

#endif
