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
#include <amqpprox_maphostnamecontrolcommand.h>

#include <amqpprox_control.h>
#include <amqpprox_dnshostnamemapper.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>

#include <memory>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

std::string MapHostnameControlCommand::commandVerb() const
{
    return "MAPHOSTNAME";
}

std::string MapHostnameControlCommand::helpText() const
{
    return "DNS - Set up mapping of IPs to hostnames";
}

void MapHostnameControlCommand::handleCommand(
    const std::string &,
    const std::string   &restOfCommand,
    const OutputFunctor &outputFunctor,
    Server              *serverHandle,
    Control             *controlHandle)
{
    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);
        if (subcommand != "DNS") {
            outputFunctor("Only DNS subcommand is supported.\n", true);
            return;
        }
    }
    else {
        outputFunctor("No subcommand specified.\n", true);
        return;
    }

    auto m = std::make_shared<DNSHostnameMapper>();
    serverHandle->setHostnameMapper(m);
    serverHandle->visitSessions(
        [&m, controlHandle](const std::shared_ptr<Session> &s) {
            s->state().setHostnameMapper(controlHandle->ioService(), m);
        });

    outputFunctor("Hostname mapper set for all current sessions.\n", true);
}

}  // namespace amqpprox
}  // namespace Bloomberg
