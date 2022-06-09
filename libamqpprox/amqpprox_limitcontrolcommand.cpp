/*
** Copyright 2022 Bloomberg Finance L.P.
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
#include <amqpprox_limitcontrolcommand.h>

#include <amqpprox_connectionlimitermanager.h>
#include <amqpprox_fixedwindowconnectionratelimiter.h>
#include <amqpprox_server.h>

#include <optional>
#include <sstream>
#include <string>
#include <tuple>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

namespace {

void handleConnectionLimitAlarm(
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    ConnectionLimiterManager *connectionLimiterManager,
    bool                      isDefault,
    const std::string        &vhostName,
    bool                      isDisable)
{
    if (isDisable) {
        if (isDefault) {
            connectionLimiterManager
                ->removeAlarmOnlyDefaultConnectionRateLimit();
            output << "Successfully disabled default alarm only "
                      "connection rate limit\n ";
        }
        else {
            connectionLimiterManager->removeAlarmOnlyConnectionRateLimiter(
                vhostName);
            output << "Successfully disabled specific alarm only "
                      "connection rate limit for vhost "
                   << vhostName << "\n";
        }
    }
    else {
        uint32_t numberOfConnections;
        if (!(iss >> numberOfConnections)) {
            output << "Invalid numberOfConnections provided.\n";
            return;
        }

        if (isDefault) {
            connectionLimiterManager->setAlarmOnlyDefaultConnectionRateLimit(
                numberOfConnections);
            output << "Default connection rate limit is set to "
                   << connectionLimiterManager
                          ->getAlarmOnlyDefaultConnectionRateLimit()
                          .value()
                   << " connections per second in alarm only mode.\n";
            output << "The limiter will only log at warning level with "
                      "AMQPPROX_CONNECTION_LIMIT as a substring and the "
                      "relevant limit details, when the new incoming "
                      "connection violates the default limit for all "
                      "vhosts.\n";
        }
        else {
            output << "For vhost " << vhostName << ", "
                   << connectionLimiterManager
                          ->addAlarmOnlyConnectionRateLimiter(
                              vhostName, numberOfConnections)
                          ->toString()
                   << " in alarm only mode.\n";
            output << "The limiter will only log at warning level with "
                      "AMQPPROX_CONNECTION_LIMIT as a substring and the "
                      "relevant limit details, when the new incoming "
                      "connection violates the specified limit.\n";
        }
    }
}

void handleConnectionLimit(
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    ConnectionLimiterManager *connectionLimiterManager,
    bool                      isDefault,
    const std::string        &vhostName,
    bool                      isDisable)
{
    if (isDisable) {
        if (isDefault) {
            connectionLimiterManager->removeDefaultConnectionRateLimit();
            output << "Successfully disabled default connection rate "
                      "limit\n ";
        }
        else {
            connectionLimiterManager->removeConnectionRateLimiter(vhostName);
            output << "Successfully disabled specific connection rate "
                      "limit for vhost "
                   << vhostName << "\n";
        }
    }
    else {
        uint32_t numberOfConnections;
        if (!(iss >> numberOfConnections)) {
            output << "Invalid numberOfConnections provided.\n";
            return;
        }

        if (isDefault) {
            connectionLimiterManager->setDefaultConnectionRateLimit(
                numberOfConnections);
            output << "Default connection rate limit is set to "
                   << connectionLimiterManager->getDefaultConnectionRateLimit()
                          .value()
                   << " connections per second.\n";
        }
        else {
            output << "For vhost " << vhostName << ", "
                   << connectionLimiterManager
                          ->addConnectionRateLimiter(vhostName,
                                                     numberOfConnections)
                          ->toString()
                   << "\n";
        }
    }
}

void printVhostLimits(
    const std::string        &vhostName,
    ConnectionLimiterManager *connectionLimiterManager,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output)
{
    auto alarmLimiter =
        connectionLimiterManager->getAlarmOnlyConnectionRateLimiter(vhostName);
    if (alarmLimiter) {
        output << "Alarm only limit, for vhost " << vhostName << ", "
               << alarmLimiter->toString() << ".\n";
    }

    auto limiter =
        connectionLimiterManager->getConnectionRateLimiter(vhostName);
    if (limiter) {
        output << "For vhost " << vhostName << ", " << limiter->toString()
               << ".\n";
    }

    if (!alarmLimiter && !limiter) {
        std::optional<uint32_t> alarmOnlyConnectionRateLimit =
            connectionLimiterManager->getAlarmOnlyDefaultConnectionRateLimit();
        std::optional<uint32_t> connectionRateLimit =
            connectionLimiterManager->getDefaultConnectionRateLimit();
        if (alarmOnlyConnectionRateLimit || connectionRateLimit) {
            if (alarmOnlyConnectionRateLimit) {
                output << "Alarm only limit, for vhost " << vhostName
                       << ", allow average " << *alarmOnlyConnectionRateLimit
                       << " number of connections per second.\n";
            }
            if (connectionRateLimit) {
                output << "For vhost " << vhostName << ", allow average "
                       << *connectionRateLimit
                       << " number of connections per second.\n";
            }
        }
        else {
            output << "No default connection rate limit configured "
                      "for any vhost.\n";
        }
    }
}

void printAllLimits(
    ConnectionLimiterManager *connectionLimiterManager,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output)
{
    std::optional<uint32_t> alarmOnlyConnectionRateLimit =
        connectionLimiterManager->getAlarmOnlyDefaultConnectionRateLimit();
    std::optional<uint32_t> connectionRateLimit =
        connectionLimiterManager->getDefaultConnectionRateLimit();
    if (alarmOnlyConnectionRateLimit || connectionRateLimit) {
        if (alarmOnlyConnectionRateLimit) {
            output << "Default limit for any vhost, allow average "
                   << *alarmOnlyConnectionRateLimit
                   << " connections per second in alarm only mode.\n";
        }
        if (connectionRateLimit) {
            output << "Default limit for any vhost, allow average "
                   << *connectionRateLimit << " connections per second.\n";
        }
    }
    else {
        output << "No default connection rate limit configured "
                  "for any vhost.\n";
    }
}

std::optional<std::tuple<bool, std::string>>
readVhostOrDefault(std::istringstream &iss)
{
    std::string subcommand;
    if (!(iss >> subcommand)) {
        return {};
    }

    boost::to_upper(subcommand);
    if (subcommand == "VHOST") {
        std::string vhostName;
        if (!(iss >> vhostName)) {
            return {};
        }

        return std::tuple(false, vhostName);
    }
    else if (subcommand == "DEFAULT") {
        return std::tuple(true, "");
    }

    return {};
}
}

LimitControlCommand::LimitControlCommand(
    ConnectionLimiterManager *connectionLimiterManager)
: ControlCommand()
, d_connectionLimiterManager_p(connectionLimiterManager)
{
}

std::string LimitControlCommand::commandVerb() const
{
    return "LIMIT";
}

std::string LimitControlCommand::helpText() const
{
    return "(CONN_RATE_ALARM | CONN_RATE) (VHOST vhostName "
           "numberOfConnections | DEFAULT numberOfConnections) - Configure "
           "connection rate limits (normal or alarmonly) for incoming clients "
           "connections\n"

           "LIMIT DISABLE (CONN_RATE_ALARM | CONN_RATE) (VHOST vhostName | "
           "DEFAULT) - Disable configured limit thresholds\n"

           "LIMIT PRINT [vhostName] - Print the configured default limits or "
           "specific vhost limits";
}

void LimitControlCommand::handleCommand(const std::string & /* command */,
                                        const std::string   &restOfCommand,
                                        const OutputFunctor &outputFunctor,
                                        Server *,
                                        Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    if (!(iss >> subcommand)) {
        output << "No subcommand provided for LIMIT command.\n";
        return;
    }

    boost::to_upper(subcommand);

    if (subcommand == "PRINT") {
        std::string vhostName;
        if (iss >> vhostName) {
            printVhostLimits(vhostName, d_connectionLimiterManager_p, output);
        }
        else {
            printAllLimits(d_connectionLimiterManager_p, output);
        }
        return;
    }

    bool isDisable = false;
    if (subcommand == "DISABLE") {
        if (!(iss >> subcommand)) {
            output << "No subcommand provided for LIMIT DISABLE command.\n";
            return;
        }

        isDisable = true;
    }

    auto vhostOrDefault = readVhostOrDefault(iss);
    if (!readVhostOrDefault) {
        output << "Failed to read (VHOST vhostName | DEFAULT) for "
               << subcommand;
        return;
    }

    auto [isDefault, vhostName] = vhostOrDefault.value();

    if (subcommand == "CONN_RATE_ALARM") {
        handleConnectionLimitAlarm(iss,
                                   output,
                                   d_connectionLimiterManager_p,
                                   isDefault,
                                   vhostName,
                                   isDisable);
    }
    else if (subcommand == "CONN_RATE") {
        handleConnectionLimit(iss,
                              output,
                              d_connectionLimiterManager_p,
                              isDefault,
                              vhostName,
                              isDisable);
    }
    else {
        output << "Invalid subcommand provided for LIMIT command.\n";
    }
}

}
}
