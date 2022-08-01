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
#include <amqpprox_dataratelimitmanager.h>
#include <amqpprox_fixedwindowconnectionratelimiter.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>

#include <limits>
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
    bool                      isDisable,
    bool                      isTotalConnLimit)
{
    std::string limitType =
        (isTotalConnLimit ? "total connection" : "connection rate");
    if (isDisable) {
        if (isDefault) {
            isTotalConnLimit
                ? connectionLimiterManager
                      ->removeAlarmOnlyDefaultTotalConnectionLimit()
                : connectionLimiterManager
                      ->removeAlarmOnlyDefaultConnectionRateLimit();
            output << "Successfully disabled default alarm only " << limitType
                   << " limit\n ";
        }
        else {
            isTotalConnLimit
                ? connectionLimiterManager
                      ->removeAlarmOnlyTotalConnectionLimiter(vhostName)
                : connectionLimiterManager
                      ->removeAlarmOnlyConnectionRateLimiter(vhostName);
            output << "Successfully disabled specific alarm only " << limitType
                   << " limit for vhost " << vhostName << "\n";
        }
    }
    else {
        uint32_t numberOfConnections;
        if (!(iss >> numberOfConnections)) {
            output << "Invalid numberOfConnections provided for " << limitType
                   << " limit.\n";
            return;
        }

        if (isDefault) {
            if (isTotalConnLimit) {
                connectionLimiterManager
                    ->setAlarmOnlyDefaultConnectionRateLimit(
                        numberOfConnections);
                output << "Default " << limitType << " limit is set to "
                       << connectionLimiterManager
                              ->getAlarmOnlyDefaultTotalConnectionLimit()
                              .value()
                       << " total connections in alarm only mode.\n";
            }
            else {
                connectionLimiterManager
                    ->setAlarmOnlyDefaultTotalConnectionLimit(
                        numberOfConnections);
                output << "Default " << limitType << " limit is set to "
                       << connectionLimiterManager
                              ->getAlarmOnlyDefaultConnectionRateLimit()
                              .value()
                       << " connections per second in alarm only mode.\n";
            }

            output
                << "The limiter will only log at warning level with "
                   "AMQPPROX_CONNECTION_LIMIT as a substring and the "
                   "relevant limit details, when the new incoming client "
                   "connection violates the default limit for all vhosts.\n";
        }
        else {
            output << "For vhost " << vhostName << ", "
                   << (isTotalConnLimit
                           ? connectionLimiterManager
                                 ->addAlarmOnlyTotalConnectionLimiter(
                                     vhostName, numberOfConnections)
                                 ->toString()
                           : connectionLimiterManager
                                 ->addAlarmOnlyConnectionRateLimiter(
                                     vhostName, numberOfConnections)
                                 ->toString())
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
    bool                      isDisable,
    bool                      isTotalConnLimit)
{
    std::string limitType =
        (isTotalConnLimit ? "total connection" : "connection rate");
    if (isDisable) {
        if (isDefault) {
            isTotalConnLimit
                ? connectionLimiterManager->removeDefaultConnectionRateLimit()
                : connectionLimiterManager
                      ->removeDefaultTotalConnectionLimit();
            output << "Successfully disabled default " << limitType
                   << " limit\n ";
        }
        else {
            isTotalConnLimit
                ? connectionLimiterManager->removeTotalConnectionLimiter(
                      vhostName)
                : connectionLimiterManager->removeConnectionRateLimiter(
                      vhostName);
            output << "Successfully disabled specific " << limitType
                   << " limit for vhost " << vhostName << "\n";
        }
    }
    else {
        uint32_t numberOfConnections;
        if (!(iss >> numberOfConnections)) {
            output << "Invalid numberOfConnections provided for " << limitType
                   << " limit\n";
            return;
        }

        if (isDefault) {
            if (isTotalConnLimit) {
                connectionLimiterManager->setDefaultTotalConnectionLimit(
                    numberOfConnections);
                output << "Default " << limitType << " limit is set to "
                       << connectionLimiterManager
                              ->getDefaultTotalConnectionLimit()
                              .value()
                       << " total connections.\n";
            }
            else {
                connectionLimiterManager->setDefaultConnectionRateLimit(
                    numberOfConnections);
                output << "Default " << limitType << " limit is set to "
                       << connectionLimiterManager
                              ->getDefaultConnectionRateLimit()
                              .value()
                       << " connections per second.\n";
            }
        }
        else {
            output << "For vhost " << vhostName << ", "
                   << (isTotalConnLimit
                           ? connectionLimiterManager
                                 ->addTotalConnectionLimiter(
                                     vhostName, numberOfConnections)
                                 ->toString()
                           : connectionLimiterManager
                                 ->addConnectionRateLimiter(
                                     vhostName, numberOfConnections)
                                 ->toString())
                   << "\n";
        }
    }
}

void updateVhostLimits(Server *serverHandle, const std::string &vhost)
{
    auto visitor = [&vhost](const std::shared_ptr<Session> &session) {
        if (session->state().getVirtualHost() == vhost) {
            session->updateDataRateLimits();
        }
    };

    serverHandle->visitSessions(visitor);
}

void updateDefaultLimits(Server *serverHandle)
{
    auto visitor = [](const std::shared_ptr<Session> &session) {
        session->updateDataRateLimits();
    };

    serverHandle->visitSessions(visitor);
}

void handleDataRateAlarmLimit(
    Server                                              *serverHandle,
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    DataRateLimitManager                                *limitManager,
    bool                                                 isDefault,
    const std::string                                   &vhostName,
    bool                                                 isDisable)
{
    if (isDisable) {
        if (isDefault) {
            const size_t DISABLED_LIMIT = std::numeric_limits<size_t>::max();
            limitManager->setDefaultDataRateAlarm(DISABLED_LIMIT);

            updateDefaultLimits(serverHandle);
        }
        else {
            limitManager->disableVhostDataRateAlarm(vhostName);

            updateVhostLimits(serverHandle, vhostName);
        }
    }
    else {
        size_t bytesPerSecond = 0;
        if (!(iss >> bytesPerSecond)) {
            output << "Failed to read bytesPerSecond";
            return;
        }

        if (isDefault) {
            limitManager->setDefaultDataRateAlarm(bytesPerSecond);

            updateDefaultLimits(serverHandle);
        }
        else {
            limitManager->setVhostDataRateAlarm(vhostName, bytesPerSecond);

            updateVhostLimits(serverHandle, vhostName);
        }
    }
}

void handleDataRateLimit(
    Server                                              *serverHandle,
    std::istringstream                                  &iss,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output,
    DataRateLimitManager                                *limitManager,
    bool                                                 isDefault,
    const std::string                                   &vhostName,
    bool                                                 isDisable)
{
    if (isDisable) {
        if (isDefault) {
            const size_t DISABLED_LIMIT = std::numeric_limits<size_t>::max();
            limitManager->setDefaultDataRateLimit(DISABLED_LIMIT);

            updateDefaultLimits(serverHandle);
        }
        else {
            limitManager->disableVhostDataRateLimit(vhostName);

            updateVhostLimits(serverHandle, vhostName);
        }
    }
    else {
        size_t bytesPerSecond = 0;
        if (!(iss >> bytesPerSecond)) {
            output << "Failed to read bytesPerSecond";
            return;
        }

        if (isDefault) {
            limitManager->setDefaultDataRateLimit(bytesPerSecond);

            updateDefaultLimits(serverHandle);
        }
        else {
            limitManager->setVhostDataRateLimit(vhostName, bytesPerSecond);

            updateVhostLimits(serverHandle, vhostName);
        }
    }
}

void printVhostLimits(
    const std::string        &vhostName,
    ConnectionLimiterManager *connectionLimiterManager,
    DataRateLimitManager     *dataRateLimitManager,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output)
{
    bool anyConfiguredLimit = false;

    auto alarmConnRateLimiter =
        connectionLimiterManager->getAlarmOnlyConnectionRateLimiter(vhostName);
    if (alarmConnRateLimiter) {
        output << "Alarm only limit, for vhost " << vhostName << ", "
               << alarmConnRateLimiter->toString() << ".\n";
        anyConfiguredLimit = true;
    }
    else {
        std::optional<uint32_t> alarmConnRateLimit =
            connectionLimiterManager->getAlarmOnlyDefaultConnectionRateLimit();
        if (alarmConnRateLimit) {
            output << "Alarm only limit, for vhost " << vhostName
                   << ", allow average " << *alarmConnRateLimit
                   << " number of connections per second.\n";
            anyConfiguredLimit = true;
        }
    }
    auto connRateLimiter =
        connectionLimiterManager->getConnectionRateLimiter(vhostName);
    if (connRateLimiter) {
        output << "For vhost " << vhostName << ", "
               << connRateLimiter->toString() << ".\n";
        anyConfiguredLimit = true;
    }
    else {
        std::optional<uint32_t> connRateLimit =
            connectionLimiterManager->getDefaultConnectionRateLimit();
        if (connRateLimit) {
            output << "For vhost " << vhostName << ", allow average "
                   << *connRateLimit << " number of connections per second.\n";
            anyConfiguredLimit = true;
        }
    }

    auto alarmTotalConnLimiter =
        connectionLimiterManager->getAlarmOnlyTotalConnectionLimiter(
            vhostName);
    if (alarmTotalConnLimiter) {
        output << "Alarm only limit, for vhost " << vhostName << ", "
               << alarmTotalConnLimiter->toString() << ".\n";
        anyConfiguredLimit = true;
    }
    else {
        std::optional<uint32_t> alarmTotalConnLimit =
            connectionLimiterManager->getAlarmOnlyDefaultConnectionRateLimit();
        if (alarmTotalConnLimit) {
            output << "Alarm only limit, for vhost " << vhostName << ", allow "
                   << *alarmTotalConnLimit << " total connections.\n";
            anyConfiguredLimit = true;
        }
    }
    auto totalConnLimiter =
        connectionLimiterManager->getTotalConnectionLimiter(vhostName);
    if (totalConnLimiter) {
        output << "For vhost " << vhostName << ", "
               << totalConnLimiter->toString() << ".\n";
        anyConfiguredLimit = true;
    }
    else {
        std::optional<uint32_t> totalConnLimit =
            connectionLimiterManager->getDefaultTotalConnectionLimit();
        if (totalConnLimit) {
            output << "For vhost " << vhostName << ", allow "
                   << *totalConnLimit << " total connections.\n";
            anyConfiguredLimit = true;
        }
    }

    std::size_t alarmDataRateLimit =
        dataRateLimitManager->getDataRateAlarm(vhostName);
    if (alarmDataRateLimit != std::numeric_limits<std::size_t>::max()) {
        output << "Alarm only data limit, for vhost " << vhostName
               << ", allow max " << alarmDataRateLimit
               << " bytes per second.\n";
        anyConfiguredLimit = true;
    }

    std::size_t dataRateLimit =
        dataRateLimitManager->getDataRateLimit(vhostName);
    if (dataRateLimit != std::numeric_limits<std::size_t>::max()) {
        output << "For vhost " << vhostName << ", allow max " << dataRateLimit
               << " bytes per second.\n";
        anyConfiguredLimit = true;
    }

    if (!anyConfiguredLimit) {
        output << "No limit configured for vhost " << vhostName << ".\n";
    }
}

void printAllLimits(
    ConnectionLimiterManager *connectionLimiterManager,
    DataRateLimitManager     *dataRateLimitManager,
    ControlCommandOutput<ControlCommand::OutputFunctor> &output)
{
    std::optional<uint32_t> alarmOnlyConnectionRateLimit =
        connectionLimiterManager->getAlarmOnlyDefaultConnectionRateLimit();
    std::optional<uint32_t> connectionRateLimit =
        connectionLimiterManager->getDefaultConnectionRateLimit();

    std::optional<uint32_t> alarmOnlyTotalConnectionLimit =
        connectionLimiterManager->getAlarmOnlyDefaultTotalConnectionLimit();
    std::optional<uint32_t> totalConnectionLimit =
        connectionLimiterManager->getDefaultTotalConnectionLimit();

    std::size_t alarmOnlyDataRateLimit =
        dataRateLimitManager->getDefaultDataRateAlarm();
    std::size_t dataRateLimit =
        dataRateLimitManager->getDefaultDataRateLimit();

    bool anyConfiguredLimit = false;
    if (alarmOnlyConnectionRateLimit) {
        output << "Default limit for any vhost, allow average "
               << *alarmOnlyConnectionRateLimit
               << " connections per second in alarm only mode.\n";
        anyConfiguredLimit = true;
    }
    if (connectionRateLimit) {
        output << "Default limit for any vhost, allow average "
               << *connectionRateLimit << " connections per second.\n";
        anyConfiguredLimit = true;
    }

    if (alarmOnlyTotalConnectionLimit) {
        output << "Default limit for any vhost, allow "
               << *alarmOnlyTotalConnectionLimit
               << " total connections in alarm only mode.\n";
        anyConfiguredLimit = true;
    }
    if (totalConnectionLimit) {
        output << "Default limit for any vhost, allow "
               << *totalConnectionLimit << " total connections.\n";
        anyConfiguredLimit = true;
    }

    if (alarmOnlyDataRateLimit != std::numeric_limits<std::size_t>::max()) {
        output << "Default data limit for any vhost, allow max "
               << alarmOnlyDataRateLimit
               << " bytes per second in alarm only mode.\n";
        anyConfiguredLimit = true;
    }
    if (dataRateLimit != std::numeric_limits<std::size_t>::max()) {
        output << "Default data limit for any vhost, allow max "
               << dataRateLimit << " bytes per second.\n";
        anyConfiguredLimit = true;
    }

    if (!anyConfiguredLimit) {
        output << "No default limit configured for any vhost.\n";
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
    ConnectionLimiterManager *connectionLimiterManager,
    DataRateLimitManager     *dataRateLimitManager)
: ControlCommand()
, d_connectionLimiterManager_p(connectionLimiterManager)
, d_dataRateLimitManager(dataRateLimitManager)
{
}

std::string LimitControlCommand::commandVerb() const
{
    return "LIMIT";
}

std::string LimitControlCommand::helpText() const
{
    return "(CONN_RATE_ALARM | CONN_RATE) (DEFAULT | VHOST vhostName) "
           "numberOfConnections - Configure connection rate limits (normal or "
           "alarmonly) for incoming clients connections\n"

           "LIMIT (TOTAL_CONN_ALARM | TOTAL_CONN) (DEFAULT | VHOST vhostName) "
           "numberOfConnections - Configure total connection limits or alarms "
           "for incoming client connections\n"

           "LIMIT (DATA_RATE_ALARM | DATA_RATE) (DEFAULT | VHOST vhostName) "
           "BytesPerSecond - Configure data rate limits or alarms for "
           "incoming client data\n"

           "LIMIT DISABLE (CONN_RATE_ALARM | CONN_RATE | TOTAL_CONN_ALARM | "
           "TOTAL_CONN | DATA_RATE_ALARM | DATA_RATE) (VHOST vhostName | "
           "DEFAULT) - Disable configured limit thresholds\n"

           "LIMIT PRINT [vhostName] - Print the configured default limits or "
           "specific vhost limits";
}

void LimitControlCommand::handleCommand(const std::string & /* command */,
                                        const std::string   &restOfCommand,
                                        const OutputFunctor &outputFunctor,
                                        Server              *serverHandle,
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
            printVhostLimits(vhostName,
                             d_connectionLimiterManager_p,
                             d_dataRateLimitManager,
                             output);
        }
        else {
            printAllLimits(
                d_connectionLimiterManager_p, d_dataRateLimitManager, output);
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
    if (!vhostOrDefault) {
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
                                   isDisable,
                                   false);
    }
    else if (subcommand == "CONN_RATE") {
        handleConnectionLimit(iss,
                              output,
                              d_connectionLimiterManager_p,
                              isDefault,
                              vhostName,
                              isDisable,
                              false);
    }
    else if (subcommand == "TOTAL_CONN_ALARM") {
        handleConnectionLimitAlarm(iss,
                                   output,
                                   d_connectionLimiterManager_p,
                                   isDefault,
                                   vhostName,
                                   isDisable,
                                   true);
    }
    else if (subcommand == "TOTAL_CONN") {
        handleConnectionLimit(iss,
                              output,
                              d_connectionLimiterManager_p,
                              isDefault,
                              vhostName,
                              isDisable,
                              true);
    }
    else if (subcommand == "DATA_RATE_ALARM") {
        handleDataRateAlarmLimit(serverHandle,
                                 iss,
                                 output,
                                 d_dataRateLimitManager,
                                 isDefault,
                                 vhostName,
                                 isDisable);
    }
    else if (subcommand == "DATA_RATE") {
        handleDataRateLimit(serverHandle,
                            iss,
                            output,
                            d_dataRateLimitManager,
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
