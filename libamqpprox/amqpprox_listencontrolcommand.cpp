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
#include <amqpprox_listencontrolcommand.h>

#include <amqpprox_server.h>

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

ListenControlCommand::ListenControlCommand()
{
}

std::string ListenControlCommand::commandVerb() const
{
    return "LISTEN";
}

std::string ListenControlCommand::helpText() const
{
    return "START port | START_SECURE port | STOP [port]";
}

void ListenControlCommand::handleCommand(const std::string & /* command */,
                                         const std::string &  restOfCommand,
                                         const OutputFunctor &outputFunctor,
                                         Server *             serverHandle,
                                         Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);

        if (subcommand == "START" || subcommand == "START_SECURE") {
            int port = -1;
            if (iss >> port && port > 0 && port <= 65535) {
                serverHandle->startListening(port,
                                             subcommand == "START_SECURE");
            }
            else {
                output << "Invalid port provided.\n";
            }
        }
        else if (subcommand == "STOP") {
            int port = -1;
            if (iss >> port && port > 0 && port <= 65535) {
                serverHandle->stopListening(port);
            }
            else {
                serverHandle->stopAllListening();
            }
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
