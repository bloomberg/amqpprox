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
#include <amqpprox_statcontrolcommand.h>

#include <amqpprox_control.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_humanstatformatter.h>
#include <amqpprox_jsonstatformatter.h>
#include <amqpprox_logging.h>
#include <amqpprox_statsdpublisher.h>
#include <amqpprox_statsnapshot.h>

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>

namespace Bloomberg {
namespace amqpprox {
namespace {

bool mapForFilter(StatSnapshot::StatsMap *map,
                  const std::string      &filterType,
                  const StatSnapshot     &statSnapshot)
{
    if (filterType == "VHOST") {
        *map = statSnapshot.vhosts();
    }
    else if (filterType == "SOURCE") {
        *map = statSnapshot.sources();
    }
    else if (filterType == "BACKEND") {
        *map = statSnapshot.backends();
    }
    else {
        return false;
    }
    return true;
}

void formatOutput(std::ostream       &oss,
                  StatFormatter      &formatter,
                  const StatSnapshot &statSnapshot,
                  const std::string  &filterType,
                  const std::string  &filterValue)
{
    StatSnapshot::StatsMap map;

    if (filterType == "ALL") {
        formatter.format(oss, statSnapshot);
    }
    else if (filterType == "OVERALL") {
        formatter.format(oss, statSnapshot.overall());
    }
    else if (filterType == "PROCESS") {
        formatter.format(oss, statSnapshot.process());
    }
    else if (filterType == "BUFFERPOOL") {
        formatter.format(
            oss, statSnapshot.pool(), statSnapshot.poolSpillover());
    }
    else if (mapForFilter(&map, filterType, statSnapshot)) {
        auto it = map.find(filterValue);
        if (it != std::end(map)) {
            formatter.format(oss, it->second);
        }
        else {
            ConnectionStats cs;
            formatter.format(oss, cs);
        }
    }
    else {
        oss << "Unknown filter type: '" << filterType << "'";
    }
    oss << std::endl;
}

bool outputStats(const StatSnapshot           &statSnapshot,
                 const std::string            &outputType,
                 const std::string            &filterType,
                 const std::string            &filterValue,
                 ControlCommand::OutputFunctor outputFunc)
{
    std::ostringstream oss;

    if (outputType == "HUMAN") {
        HumanStatFormatter formatter;
        formatOutput(oss, formatter, statSnapshot, filterType, filterValue);
    }
    else if (outputType == "JSON") {
        JsonStatFormatter formatter;
        formatOutput(oss, formatter, statSnapshot, filterType, filterValue);
    }
    else {
        outputFunc("Unknown output format.\n", false);
        LOG_INFO << "Stopping stat listening for: " << outputType << " "
                 << filterType << "=" << filterValue;
        return false;
    }

    bool cont = outputFunc(oss.str(), false);
    if (!cont) {
        LOG_INFO << "Stopping stat listening for: " << outputType << " "
                 << filterType << "=" << filterValue;
    }

    return cont;
}

using StatFunctor = std::function<bool(const StatSnapshot &)>;
void stopSendStats(std::list<std::pair<StatFunctor, bool>> *d_functors)
{
    auto it = std::begin(*d_functors);
    for (; it != std::end(*d_functors);) {
        auto functor = *it;
        if (functor.second) {
            it = d_functors->erase(it);
        }
        else {
            ++it;
        }
    }
}

}

StatControlCommand::StatControlCommand(EventSource *eventSource)
: d_functors()
, d_statisticsAvailableSignal()
, d_eventSource_p(eventSource)
{
    d_statisticsAvailableSignal =
        d_eventSource_p->statisticsAvailable().subscribe(std::bind(
            &StatControlCommand::invokeHandlers, this, std::placeholders::_1));
}

void StatControlCommand::invokeHandlers(StatCollector *collector)
{
    StatSnapshot snapshot;
    collector->populateStats(&snapshot);

    auto it = std::begin(d_functors);
    for (; it != std::end(d_functors);) {
        auto functor = *it;
        if (!functor.first(snapshot)) {
            it = d_functors.erase(it);
        }
        else {
            ++it;
        }
    }
}

std::string StatControlCommand::commandVerb() const
{
    return "STAT";
}

std::string StatControlCommand::helpText() const
{
    return "(STOP SEND | SEND <host> <port> | (LISTEN (json|human) "
           "(overall|vhost=foo|backend=bar|source=baz|all|process|bufferpool))"
           " - "
           "Output statistics";
}

void StatControlCommand::handleCommand(const std::string   &command,
                                       const std::string   &restOfCommand,
                                       const OutputFunctor &outputFunctor,
                                       Server              *serverHandle,
                                       Control             *controlHandle)
{
    std::istringstream iss(restOfCommand);
    std::string        subcommand;

    iss >> subcommand;
    boost::to_upper(subcommand);

    std::string outputType;
    std::string outputHost;
    int         outputPort;
    if (subcommand == "LISTEN") {
        if (iss >> outputType) {
            boost::to_upper(outputType);
            if (outputType != "JSON" && outputType != "HUMAN") {
                outputFunctor("json or human must be the type of output\n",
                              true);
                return;
            }
        }
        else {
            outputFunctor("No output type specified.\n", true);
            return;
        }
    }
    else if (subcommand == "SEND") {
        if (!(iss >> outputHost)) {
            outputFunctor("No output host specified.\n", true);
            return;
        }
        if (!(iss >> outputPort)) {
            outputFunctor("No output port specified.\n", true);
            return;
        }
    }
    else if (subcommand == "STOP") {
        std::string type;
        if (!(iss >> type)) {
            outputFunctor("Missing stop type.\n", true);
            return;
        }
        if (type == "SEND") {
            stopSendStats(&d_functors);
            outputFunctor("Stopped sending stats.\n", true);
        }
        else {
            outputFunctor(
                "Unrecognized argument for STOP: \"" + type + "\".\n", true);
        }
        return;
    }
    else {
        outputFunctor(
            "Only LISTEN, SEND and STOP subcommands are supported.\n", true);
        return;
    }

    std::string filterType  = "ALL";
    std::string filterValue = "";

    std::string filterTerm;
    if (iss >> filterTerm) {
        std::string uppercasedFilterTerm(filterTerm);
        boost::to_upper(uppercasedFilterTerm);
        if (uppercasedFilterTerm == "ALL" ||
            uppercasedFilterTerm == "OVERALL" ||
            uppercasedFilterTerm == "BUFFERPOOL" ||
            uppercasedFilterTerm == "PROCESS") {
            filterType = uppercasedFilterTerm;
        }
        else {
            boost::tokenizer<boost::char_separator<char>> tokenizer(
                filterTerm, boost::char_separator<char>("="));

            std::vector<std::string> tokens;
            for (const auto &tok : tokenizer) {
                tokens.push_back(tok);
            }

            if (tokens.size() != 2) {
                outputFunctor("Filter specified incorrectly.\n", true);
                return;
            }

            filterType  = tokens[0];
            filterValue = tokens[1];
            boost::to_upper(filterType);
        }
    }

    LOG_INFO << "Begin stat "
             << (subcommand == "LISTEN" ? "listening" : "sending")
             << outputType << " " << filterType << "=" << filterValue;

    if (subcommand == "LISTEN") {
        StatFunctor sf = std::bind(&outputStats,
                                   std::placeholders::_1,
                                   outputType,
                                   filterType,
                                   filterValue,
                                   outputFunctor);
        d_functors.push_back({sf, false});
    }
    else {
        if (filterType != "ALL") {
            outputFunctor(
                "Filters are currently not supported when sending metrics.\n",
                true);
            return;
        }
        std::shared_ptr<StatsDPublisher> publisher =
            std::make_shared<StatsDPublisher>(
                &controlHandle->ioContext(), outputHost, outputPort);
        StatFunctor sf = [publisher](const StatSnapshot &statSnapshot) {
            publisher->publish(statSnapshot);
            return true;
        };

        d_functors.push_back({sf, true});
        outputFunctor("Sending stats on " + outputHost + ":" +
                          std::to_string(outputPort) + "\n",
                      true);
    }
}

}
}
