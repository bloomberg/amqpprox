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
#ifndef BLOOMBERG_AMQPPROX_STATSNAPSHOT
#define BLOOMBERG_AMQPPROX_STATSNAPSHOT

#include <amqpprox_connectionstats.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Value type for a snapshot of all statistics from the program.
 *
 * This component provides a value type for encapsulating all the statistics
 * that the program collects.
 */
class StatSnapshot {
  public:
    using StatsMap = std::unordered_map<std::string, ConnectionStats>;

    struct ProcessStats {
        uint64_t d_rssKB;
        uint16_t d_user;
        uint16_t d_system;
        uint16_t d_overall;

        ProcessStats()
        : d_rssKB(0)
        , d_user(0)
        , d_system(0)
        , d_overall(0)
        {
        }
    };

    struct PoolStats {
        std::size_t d_bufferSize;
        uint64_t    d_highwaterMark;
        uint64_t    d_currentAllocation;

        PoolStats()
        : d_bufferSize(0)
        , d_highwaterMark(0)
        , d_currentAllocation(0)
        {
        }
    };

  private:
    StatsMap               d_vhosts;
    StatsMap               d_sources;
    StatsMap               d_backends;
    ConnectionStats        d_overallConnectionStats;
    ProcessStats           d_process;
    std::vector<PoolStats> d_pool;
    uint64_t               d_poolSpillover;

  public:
    // CREATORS
    StatSnapshot();

    // ACCESSORS
    /**
     * \return reference to StatsMap for vhosts
     */
    inline StatsMap &vhosts();
    /**
     * \return const reference to StatsMap for vhosts
     */
    inline const StatsMap &vhosts() const;

    /**
     * \return reference to StatsMap for sources
     */
    inline StatsMap &sources();
    /**
     * \return const reference to StatsMap for sources
     */
    inline const StatsMap &sources() const;

    /**
     * \return reference to StatsMap for backends
     */
    inline StatsMap &backends();
    /**
     * \return const reference to StatsMap for backends
     */
    inline const StatsMap &backends() const;

    /**
     * \return reference to overall ConnectionStats
     */
    inline ConnectionStats &overall();
    /**
     * \return const reference to overall ConnectionStats
     */
    inline const ConnectionStats &overall() const;

    /**
     * \return reference to ProcessStats
     */
    inline ProcessStats &process();
    /**
     * \return const reference to ProcessStats
     */
    inline const ProcessStats &process() const;

    /**
     * \return reference to vector of PoolStats
     */
    inline std::vector<PoolStats> &pool();
    /**
     * \return const reference to vector of PoolStats
     */
    inline const std::vector<PoolStats> &pool() const;

    /**
     * \return reference to the amount of poolSpillover
     */
    inline uint64_t &poolSpillover();
    /**
     * \return const reference to the amount of poolSpillover
     */
    inline const uint64_t &poolSpillover() const;

    // MANIPULATORS
    /**
     * \brief swap the current StatSnapshot with supplied StatSnapshot
     * \param rhs StatSnapshot whose value will be replaced with current
     * StatSnapshot
     */
    void swap(StatSnapshot &rhs);
};

inline StatSnapshot::StatsMap &StatSnapshot::vhosts()
{
    return d_vhosts;
}

inline const StatSnapshot::StatsMap &StatSnapshot::vhosts() const
{
    return d_vhosts;
}

inline StatSnapshot::StatsMap &StatSnapshot::sources()
{
    return d_sources;
}

inline const StatSnapshot::StatsMap &StatSnapshot::sources() const
{
    return d_sources;
}

inline StatSnapshot::StatsMap &StatSnapshot::backends()
{
    return d_backends;
}

inline const StatSnapshot::StatsMap &StatSnapshot::backends() const
{
    return d_backends;
}

inline ConnectionStats &StatSnapshot::overall()
{
    return d_overallConnectionStats;
}

inline const ConnectionStats &StatSnapshot::overall() const
{
    return d_overallConnectionStats;
}

inline StatSnapshot::ProcessStats &StatSnapshot::process()
{
    return d_process;
}

inline const StatSnapshot::ProcessStats &StatSnapshot::process() const
{
    return d_process;
}

inline std::vector<StatSnapshot::PoolStats> &StatSnapshot::pool()
{
    return d_pool;
}

inline const std::vector<StatSnapshot::PoolStats> &StatSnapshot::pool() const
{
    return d_pool;
}

inline uint64_t &StatSnapshot::poolSpillover()
{
    return d_poolSpillover;
}

inline const uint64_t &StatSnapshot::poolSpillover() const
{
    return d_poolSpillover;
}

bool operator==(const StatSnapshot::ProcessStats &lhs,
                const StatSnapshot::ProcessStats &rhs);
bool operator!=(const StatSnapshot::ProcessStats &lhs,
                const StatSnapshot::ProcessStats &rhs);

}
}

#endif
