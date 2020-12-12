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
#include <amqpprox_statsdpublisher.h>

#include <amqpprox_connectionstats.h>
#include <amqpprox_logging.h>

namespace Bloomberg {
namespace amqpprox {

namespace {

enum class MetricType { GAUGE, COUNTER, DISTRIBUTION };

template <typename T>
std::string
formatMetric(MetricType                                              type,
             const std::string &                                     name,
             T                                                       value,
             const std::vector<std::pair<std::string, std::string>> &tags)
{
    std::string metric = "amqpprox.";
    metric += name;
    for (const auto &tag : tags) {
        metric += ",";
        metric += tag.first;
        metric += "=";
        metric += tag.second;
    }
    metric += ":";
    metric += std::to_string(value);
    metric += "|";
    if (type == MetricType::GAUGE) {
        metric += "g";
    }
    else if (type == MetricType::COUNTER) {
        metric += "c";
    }
    else if (type == MetricType::DISTRIBUTION) {
        metric += "d";
    }

    return metric;
}

}

StatsDPublisher::StatsDPublisher(boost::asio::io_service *ioService,
                                 const std::string &      host,
                                 int                      port)
: d_socket(*ioService,
           boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0))
{
    boost::asio::ip::udp::resolver        resolver(*ioService);
    boost::asio::ip::udp::resolver::query query(
        boost::asio::ip::udp::v4(), host, std::to_string(port));
    d_statsdEndpoint = *resolver.resolve(query);
}

void StatsDPublisher::sendMetric(const std::string &metric)
{
    auto writeHandler = [metric](const boost::system::error_code &ec,
                                 std::size_t bytesTransferred) {
        if (ec) {
            LOG_WARN << "Failed to send metric: " << metric
                     << "error code: " << ec;
        }
    };
    d_socket.async_send_to(
        boost::asio::buffer(metric), d_statsdEndpoint, writeHandler);
}

void StatsDPublisher::publish(const ConnectionStats &stats,
                              const TagVector &      tags)
{
    static const std::vector<std::string> gaugeMetrics = {
        "pausedConnectionCount", "activeConnectionCount"};
    for (auto &name : ConnectionStats::statsTypes()) {
        MetricType type = MetricType::COUNTER;

        if (std::find(gaugeMetrics.begin(), gaugeMetrics.end(), name) !=
            gaugeMetrics.end()) {
            type = MetricType::GAUGE;
        }
        sendMetric(formatMetric(type, name, stats.statsValue(name), tags));
    }

    MetricType type = MetricType::DISTRIBUTION;
    for (auto &name : ConnectionStats::distributionMetrics()) {
        if (stats.distributionCount(name) > 0) {
            sendMetric(
                formatMetric(type, name, stats.distributionValue(name), tags));
        }
    }
}

void StatsDPublisher::publish(const StatSnapshot::ProcessStats &stats)
{
    sendMetric(formatMetric(
        MetricType::COUNTER, "cpu_percent_overall", stats.d_overall, {}));
    sendMetric(formatMetric(
        MetricType::COUNTER, "cpu_percent_user", stats.d_user, {}));
    sendMetric(formatMetric(
        MetricType::COUNTER, "cpu_percent_system", stats.d_system, {}));
    sendMetric(
        formatMetric(MetricType::COUNTER, "mem_rss_kb", stats.d_rssKB, {}));
}

void StatsDPublisher::publishVhost(const StatSnapshot::StatsMap &stats)
{
    for (const auto &vhost : stats) {
        if (vhost.first == "") {
            continue;
        }
        publish(vhost.second,
                {{"rmqEndpointType", "vhost"}, {"rmqVhostName", vhost.first}});
    }
}

void StatsDPublisher::publish(
    const std::vector<StatSnapshot::PoolStats> &poolStats,
    uint64_t                                    poolSpillover)
{
    sendMetric(formatMetric(
        MetricType::COUNTER, "spill_to_heap_count", poolSpillover, {}));
    for (const auto &pool : poolStats) {
        sendMetric(formatMetric(MetricType::COUNTER,
                                "pools_" + std::to_string(pool.d_bufferSize) +
                                    "_current",
                                pool.d_currentAllocation,
                                {}));
        sendMetric(formatMetric(MetricType::COUNTER,
                                "pools_" + std::to_string(pool.d_bufferSize) +
                                    "_highest",
                                pool.d_highwaterMark,
                                {}));
    }
}

void StatsDPublisher::publishHostnameMetrics(
    const StatSnapshot::StatsMap &stats,
    const std::string &           type)
{
    for (const auto &stat : stats) {
        publish(
            stat.second,
            {{"rmqEndpointType", type}, {"rmqEndpointHostname", stat.first}});
    }
}

void StatsDPublisher::publish(const StatSnapshot &statSnapshot)
{
    publish(statSnapshot.overall(), {{"rmqEndpointType", "overall"}});
    publish(statSnapshot.process());
    publishVhost(statSnapshot.vhosts());
    publish(statSnapshot.pool(), statSnapshot.poolSpillover());
    publishHostnameMetrics(statSnapshot.sources(), "sources");
    publishHostnameMetrics(statSnapshot.backends(), "backends");
}

}
}
