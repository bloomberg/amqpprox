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

#include <amqpprox_fixedwindowconnectionratelimiter.h>
#include <amqpprox_server.h>

#include <limits>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

namespace {

void enableConnectionRateLimitBasedOnInput(
    bool                                                 alarmOnly,
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    ConnectionLimiterManager *connectionLimiterManager)
{
    std::string subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);
        if (subcommand == "VHOST") {
            std::string vhostName;
            if (!(iss >> vhostName)) {
                output << "No vhostName specified.\n";
                return;
            }

            uint32_t numberOfConnections;
            if (!(iss >> numberOfConnections && numberOfConnections >= 0 &&
                  numberOfConnections <
                      std::numeric_limits<uint32_t>::max())) {
                output << "Invalid numberOfConnections provided.\n";
                return;
            }

            if (alarmOnly) {
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
            else {
                output << "For vhost " << vhostName << ", "
                       << connectionLimiterManager
                              ->addConnectionRateLimiter(vhostName,
                                                         numberOfConnections)
                              ->toString()
                       << "\n";
            }
        }
        else if (subcommand == "DEFAULT") {
            uint32_t numberOfConnections;
            if (!(iss >> numberOfConnections && numberOfConnections >= 0 &&
                  numberOfConnections <
                      std::numeric_limits<uint32_t>::max())) {
                output << "Invalid numberOfConnections provided.\n";
                return;
            }

            if (alarmOnly) {
                connectionLimiterManager
                    ->setAlarmOnlyDefaultConnectionRateLimit(
                        numberOfConnections);
                output << "Default connection rate limit is set to "
                       << connectionLimiterManager
                              ->getAlarmOnlyDefaultConnectionRateLimit()
                       << " connections per second in alarm only mode.\n";
                output << "The limiter will only log at warning level with "
                          "AMQPPROX_CONNECTION_LIMIT as a substring and the "
                          "relevant limit details, when the new incoming "
                          "connection violates the default limit for all "
                          "vhosts.\n";
            }
            else {
                connectionLimiterManager->setDefaultConnectionRateLimit(
                    numberOfConnections);
                output << "Default connection rate limit is set to "
                       << connectionLimiterManager
                              ->getDefaultConnectionRateLimit()
                       << " connections per second.\n";
            }
        }
        else {
            if (alarmOnly) {
                output << "Invalid subcommand provided for LIMIT "
                          "CONN_RATE_ALARM command.\n";
            }
            else {
                output << "Invalid subcommand provided for LIMIT CONN_RATE "
                          "command.\n";
            }
        }
    }
    else {
        if (alarmOnly) {
            output << "No subcommand provided for LIMIT CONN_RATE_ALARM "
                      "command.\n";
        }
        else {
            output << "No subcommand provided for LIMIT CONN_RATE command.\n";
        }
    }
}

void disableConnectionRateLimitBasedOnInput(
    bool                                                 alarmOnly,
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    ConnectionLimiterManager *connectionLimiterManager)
{
    std::string subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);
        if (subcommand == "VHOST") {
            std::string vhostName;
            if (!(iss >> vhostName)) {
                output << "No vhostName specified.\n";
                return;
            }

            if (alarmOnly) {
                connectionLimiterManager->removeAlarmOnlyConnectionRateLimiter(
                    vhostName);
                output << "Successfully disabled specific alarm only "
                          "connection rate limit for vhost "
                       << vhostName << "\n";
            }
            else {
                connectionLimiterManager->removeConnectionRateLimiter(
                    vhostName);
                output << "Successfully disabled specific connection rate "
                          "limit for vhost "
                       << vhostName << "\n";
            }
        }
        else if (subcommand == "DEFAULT") {
            if (alarmOnly) {
                connectionLimiterManager
                    ->removeAlarmOnlyDefaultConnectionRateLimit();
                output << "Successfully disabled default alarm only "
                          "connection rate limit\n ";
            }
            else {
                connectionLimiterManager->removeDefaultConnectionRateLimit();
                output << "Successfully disabled default connection rate "
                          "limit\n ";
            }
        }
        else {
            if (alarmOnly) {
                output << "Invalid subcommand provided for LIMIT DISABLE "
                          "CONN_RATE_ALARM command.\n";
            }
            else {
                output << "Invalid subcommand provided for LIMIT DISABLE "
                          "CONN_RATE command.\n";
            }
        }
    }
    else {
        if (alarmOnly) {
            output << "No subcommand provided for LIMIT DISABLE "
                      "CONN_RATE_ALARM command.\n";
        }
        else {
            output << "No subcommand provided for LIMIT DISABLE CONN_RATE "
                      "command.\n";
        }
    }
}

void handleConnectionRateLimitCommand(
    bool                                                 alarmOnly,
    bool                                                 enable,
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    ConnectionLimiterManager *connectionLimiterManager)
{
    if (enable) {
        enableConnectionRateLimitBasedOnInput(
            alarmOnly, iss, output, connectionLimiterManager);
    }
    else {
        disableConnectionRateLimitBasedOnInput(
            alarmOnly, iss, output, connectionLimiterManager);
    }
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
           "connections\nLIMIT DISABLE (CONN_RATE_ALARM | CONN_RATE) (VHOST "
           "vhostName | DEFAULT) - Disable configured connection rate limits "
           "(normal or alarmonly) for incoming clients\nLIMIT PRINT "
           "[vhostName] - Print the configured default or specific connection "
           "rate limits for specified vhost";
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
    if (iss >> subcommand) {
        boost::to_upper(subcommand);

        if (subcommand == "CONN_RATE_ALARM") {
            handleConnectionRateLimitCommand(
                true, true, iss, output, d_connectionLimiterManager_p);
        }
        else if (subcommand == "CONN_RATE") {
            handleConnectionRateLimitCommand(
                false, true, iss, output, d_connectionLimiterManager_p);
        }
        else if (subcommand == "DISABLE") {
            if (iss >> subcommand) {
                boost::to_upper(subcommand);
                if (subcommand == "CONN_RATE_ALARM") {
                    handleConnectionRateLimitCommand(
                        true,
                        false,
                        iss,
                        output,
                        d_connectionLimiterManager_p);
                }
                else if (subcommand == "CONN_RATE") {
                    handleConnectionRateLimitCommand(
                        false,
                        false,
                        iss,
                        output,
                        d_connectionLimiterManager_p);
                }
                else {
                    output << "Invalid subcommand provided for LIMIT DISABLE "
                              "command.\n";
                }
            }
            else {
                output
                    << "No subcommand provided for LIMIT DISABLE command.\n";
            }
        }
        else if (subcommand == "PRINT") {
            std::string vhostName;
            if (iss >> vhostName) {
                auto alarmLimiter =
                    d_connectionLimiterManager_p
                        ->getAlarmOnlyConnectionRateLimiter(vhostName);
                if (alarmLimiter) {
                    output << "Alarm only limit, for vhost " << vhostName
                           << ", " << alarmLimiter->toString() << ".\n";
                }

                auto limiter =
                    d_connectionLimiterManager_p->getConnectionRateLimiter(
                        vhostName);
                if (limiter) {
                    output << "For vhost " << vhostName << ", "
                           << limiter->toString() << ".\n";
                }

                if (!alarmLimiter && !limiter) {
                    uint32_t alarmOnlyConnectionRateLimit =
                        d_connectionLimiterManager_p
                            ->getAlarmOnlyDefaultConnectionRateLimit();
                    uint32_t connectionRateLimit =
                        d_connectionLimiterManager_p
                            ->getDefaultConnectionRateLimit();
                    if (alarmOnlyConnectionRateLimit || connectionRateLimit) {
                        if (alarmOnlyConnectionRateLimit) {
                            output << "Alarm only limit, for vhost "
                                   << vhostName << ", allow average "
                                   << alarmOnlyConnectionRateLimit
                                   << " number of connections per second.\n";
                        }
                        if (connectionRateLimit) {
                            output << "For vhost " << vhostName
                                   << ", allow average " << connectionRateLimit
                                   << " number of connections per second.\n";
                        }
                    }
                    else {
                        output
                            << "No default connection rate limit configured "
                               "for any vhost.\n";
                    }
                }
            }
            else {
                uint32_t alarmOnlyConnectionRateLimit =
                    d_connectionLimiterManager_p
                        ->getAlarmOnlyDefaultConnectionRateLimit();
                uint32_t connectionRateLimit =
                    d_connectionLimiterManager_p
                        ->getDefaultConnectionRateLimit();
                if (alarmOnlyConnectionRateLimit || connectionRateLimit) {
                    if (alarmOnlyConnectionRateLimit) {
                        output
                            << "Default limit for any vhost, allow average "
                            << alarmOnlyConnectionRateLimit
                            << " connections per second in alarm only mode.\n";
                    }
                    if (connectionRateLimit) {
                        output << "Default limit for any vhost, allow average "
                               << connectionRateLimit
                               << " connections per second.\n";
                    }
                }
                else {
                    output << "No default connection rate limit configured "
                              "for any vhost.\n";
                }
            }
        }
        else {
            output << "Invalid subcommand provided for LIMIT command.\n";
        }
    }
    else {
        output << "No subcommand provided for LIMIT command.\n";
    }
}

}
}
