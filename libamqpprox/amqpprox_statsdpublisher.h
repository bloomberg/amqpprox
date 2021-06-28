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
#ifndef BLOOMBERG_AMQPPROX_STATSDPUBLISHER
#define BLOOMBERG_AMQPPROX_STATSDPUBLISHER

#include <string>

#include <boost/asio.hpp>

#include <amqpprox_statsnapshot.h>

namespace Bloomberg {
namespace amqpprox {

class ConnectionStats;

/**
 * \brief Provides support to publish various statistics objects used by the
 * `StatCollector` to configured StatsD endpoint
 */
class StatsDPublisher {
    boost::asio::ip::udp::endpoint d_statsdEndpoint;
    boost::asio::ip::udp::socket   d_socket;

    void sendMetric(const std::string &metric);

    typedef std::vector<std::pair<std::string, std::string>> TagVector;

  public:
    // CREATORS
    StatsDPublisher(boost::asio::io_service *ioService,
                    const std::string &      host,
                    int                      port);

    // MANIPULATORS
    /**
     * \brief Publish `StatSnapshot` to the StatsD endpoint
     * \param statSnapshot const reference to `StatSnapshot`
     */
    void publish(const StatSnapshot &statSnapshot);

    /**
     * \brief Publish `ConnectionStats` with metric tags to the StatsD endpoint
     * \param stats const reference to `ConnectionStats`
     * \param tags const reference to the vector of metric tags
     */
    void publish(const ConnectionStats &stats, const TagVector &tags);

    /**
     * \brief Publish `StatSnapshot::ProcessStats` to the StatsD endpoint
     * \param stats const reference to `StatSnapshot::ProcessStats`
     */
    void publish(const StatSnapshot::ProcessStats &stats);

    /**
     * \brief Publish `StatSnapshot::StatsMap` to the StatsD endpoint
     * \param stats const reference to `StatSnapshot::StatsMap`
     */
    void publishVhost(const StatSnapshot::StatsMap &stats);

    /**
     * \brief Publish `StatSnapshot::PoolStats` to the StatsD endpoint
     * \param poolStats const reference to the vector of
     * `StatSnapshot::PoolStats`
     * \param poolSpillover the amount of poolSpillover
     */
    void publish(const std::vector<StatSnapshot::PoolStats> &poolStats,
                 uint64_t                                    poolSpillover);

    /**
     * \brief Publish hostname metric to the StatsD endpoint
     * \param stats const reference to `StatSnapshot::StatsMap`
     * \param type of endpoint
     */
    void publishHostnameMetrics(const StatSnapshot::StatsMap &stats,
                                const std::string &           type);
};

}
}

#endif
