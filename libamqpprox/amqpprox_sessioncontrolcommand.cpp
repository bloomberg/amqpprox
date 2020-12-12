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
#include <amqpprox_sessioncontrolcommand.h>

#include <amqpprox_control.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

std::string SessionControlCommand::commandVerb() const
{
    return "SESSION";
}

std::string SessionControlCommand::helpText() const
{
    return " id# ("
           "PAUSE|"
           "DISCONNECT_GRACEFUL|"
           "FORCE_DISCONNECT"
           ") - Control a particular session";
}

void SessionControlCommand::handleCommand(const std::string & /* command */,
                                          const std::string &  restOfCommand,
                                          const OutputFunctor &outputFunctor,
                                          Server *             serverHandle,
                                          Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    uint64_t           id = 0;
    std::istringstream iss(restOfCommand);
    iss >> id;

    std::string subcommand;
    iss >> subcommand;
    boost::to_upper(subcommand);

    auto session = serverHandle->getSession(id);
    if (!session) {
        output << "Session not found.\n";
        return;
    }

    if (subcommand == "PAUSE") {
        session->pause();
    }
    else if (subcommand == "DISCONNECT_GRACEFUL") {
        session->disconnect(false);
    }
    else if (subcommand == "FORCE_DISCONNECT") {
        session->disconnect(true);
    }
    else {
        output << "Session subcommand not found.\n";
    }
}

}
}
