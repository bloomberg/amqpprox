/*
** Copyright 2021 Bloomberg Finance L.P.
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
#include <amqpprox_authcontrolcommand.h>

#include <amqpprox_defaultauthintercept.h>
#include <amqpprox_httpauthintercept.h>
#include <amqpprox_server.h>

#include <memory>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

AuthControlCommand::AuthControlCommand()
{
}

std::string AuthControlCommand::commandVerb() const
{
    return "AUTH";
}

std::string AuthControlCommand::helpText() const
{
    return "(SERVICE hostname port target | ALWAYS_ALLOW | PRINT) - "
           "Change authentication mechanism for connecting clients";
}

void AuthControlCommand::handleCommand(const std::string & /* command */,
                                       const std::string   &restOfCommand,
                                       const OutputFunctor &outputFunctor,
                                       Server              *serverHandle,
                                       Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);

        if (subcommand == "SERVICE") {
            std::string hostname;
            int         port = -1;
            std::string target;
            if (!(iss >> hostname)) {
                output << "No hostname specified.\n";
                return;
            }
            if (!(iss >> port && port > 0 && port <= 65535)) {
                output << "Invalid port provided.\n";
                return;
            }
            if (!(iss >> target)) {
                output << "No http target specified.\n";
                return;
            }

            serverHandle->setAuthIntercept(std::make_shared<HttpAuthIntercept>(
                serverHandle->ioService(),
                hostname,
                std::to_string(port),
                target,
                serverHandle->getDNSResolverPtr()));

            serverHandle->getAuthIntercept()->print(output);
        }
        else if (subcommand == "ALWAYS_ALLOW") {
            serverHandle->setAuthIntercept(
                std::make_shared<DefaultAuthIntercept>(
                    serverHandle->ioService()));

            serverHandle->getAuthIntercept()->print(output);
        }
        else if (subcommand == "PRINT") {
            serverHandle->getAuthIntercept()->print(output);
        }
        else {
            output << "Unknown subcommand.\n";
        }
    }
    else {
        output << "No subcommand provided.\n";
    }
}

}
}
