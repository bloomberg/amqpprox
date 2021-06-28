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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONSTATS
#define BLOOMBERG_AMQPPROX_CONNECTIONSTATS

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Stores summary metrics from a set of sessions
 *
 * This component is for storing a set of summary metrics collected from
 * multiple sessions, it has counts of each lifecycle state of a session and
 * totals for packets, frames and bytes sent and received.
 */
class ConnectionStats {
  public:
    std::map<std::string, uint64_t> d_statsMap;
    // store total and count for distribution metrics
    std::map<std::string, std::pair<uint64_t, uint64_t>>
                                          d_distributionStatsMap;
    static const std::vector<std::string> s_statsTypes;
    static const std::vector<std::string> s_sessionMetrics;
    static const std::vector<std::string> s_distributionMetrics;

  public:
    // CREATORS
    /**
     * \brief Construct and zero all counters and totals
     */
    ConnectionStats();

    /**
     * \brief Construct with predetermined values.  Mostly useful for unit
     * testing
     * \param stats Stats
     * \param distributionStats Distribution stats
     */
    ConnectionStats(const std::map<std::string, uint64_t> &stats,
                    const std::map<std::string, std::pair<uint64_t, uint64_t>>
                        &distributionStats);

    // MANIPULATORS
    /**
     * \brief Swap the current connection stats object with the rhs object
     */
    void swap(ConnectionStats &rhs);

    /**
     * \brief Add the value to the distribution stats
     */
    void addDistributionStats(const std::string &name,
                              uint64_t           total,
                              uint64_t           count);

    // ACCESSORS
    /**
     * \return Reference to the stats value named by name
     */
    inline uint64_t &statsValue(const std::string &name);

    /**
     * \return const reference to the stats value named by name
     */
    inline const uint64_t &statsValue(const std::string &name) const;

    /**
     * \return Count for a distribution stat
     */
    uint64_t distributionCount(const std::string &name) const;

    /**
     * \return Average value for a distribution stat
     */
    double distributionValue(const std::string &name) const;

    /**
     * \return (count, total) pair for the distribution metric
     */
    std::pair<uint64_t, uint64_t>
    distributionPair(const std::string &name) const;

    /**
     * \return All available stats types
     */
    static const std::vector<std::string> &statsTypes()
    {
        return s_statsTypes;
    }

    /**
     * \return All traffic stats types
     */
    static const std::vector<std::string> &sessionMetrics()
    {
        return s_sessionMetrics;
    }

    /**
     * \return The metrics for which distributions are published
     */
    static const std::vector<std::string> &distributionMetrics()
    ///< Return all distribution metric types
    {
        return s_distributionMetrics;
    }

    /**
     * \return Comparison for equality of all counter and total values for the
     * stats object
     */
    bool operator==(const ConnectionStats &other) const;

    /**
     * \return Comparison for inequality of all counter and total values for
     * the stats object
     */
    bool operator!=(const ConnectionStats &other) const;
};

inline uint64_t &ConnectionStats::statsValue(const std::string &name)
{
    assert(std::find(s_statsTypes.begin(), s_statsTypes.end(), name) !=
           s_statsTypes.end());
    return d_statsMap[name];
}

inline const uint64_t &
ConnectionStats::statsValue(const std::string &name) const
{
    assert(std::find(s_statsTypes.begin(), s_statsTypes.end(), name) !=
           s_statsTypes.end());
    return d_statsMap.find(name)->second;
}

}
}

#endif
