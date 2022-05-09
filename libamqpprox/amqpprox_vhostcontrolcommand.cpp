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
#include <amqpprox_vhostcontrolcommand.h>

#include <amqpprox_server.h>
#include <amqpprox_session.h>
#include <amqpprox_vhoststate.h>

#include <sstream>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

VhostControlCommand::VhostControlCommand(VhostState *vhostState)
: d_vhostState_p(vhostState)
{
}

std::string VhostControlCommand::commandVerb() const
{
    return "VHOST";
}

std::string VhostControlCommand::helpText() const
{
    return "PAUSE vhost | "
           "UNPAUSE vhost | "
           "PRINT | "
           "BACKEND_DISCONNECT vhost | "
           "FORCE_DISCONNECT vhost";
}

void VhostControlCommand::handleCommand(const std::string & /* command */,
                                        const std::string   &restOfCommand,
                                        const OutputFunctor &outputFunctor,
                                        Server              *serverHandle,
                                        Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    iss >> subcommand;
    boost::to_upper(subcommand);

    std::string vhost;
    iss >> vhost;

    if (subcommand.empty() || (vhost.empty() && subcommand != "PRINT")) {
        output << "Session not found.\n";
        return;
    }

    if (subcommand == "PAUSE") {
        auto visitor = [this, &vhost](std::shared_ptr<Session> session) {
            if (session->state().getVirtualHost() == vhost) {
                session->pause();
            }
        };

        d_vhostState_p->setPaused(vhost, true);
        serverHandle->visitSessions(visitor);
    }
    else if (subcommand == "UNPAUSE") {
        d_vhostState_p->setPaused(vhost, false);

        auto visitor = [this, &vhost](std::shared_ptr<Session> session) {
            if (session->state().getVirtualHost() == vhost) {
                session->unpause();
            }
        };

        serverHandle->visitSessions(visitor);
    }
    else if (subcommand == "FORCE_DISCONNECT") {
        auto visitor = [this, &vhost](std::shared_ptr<Session> session) {
            if (session->state().getVirtualHost() == vhost) {
                session->disconnect(true);
            }
        };

        serverHandle->visitSessions(visitor);
    }
    else if (subcommand == "BACKEND_DISCONNECT") {
        auto visitor = [this, &vhost](std::shared_ptr<Session> session) {
            if (session->state().getVirtualHost() == vhost) {
                session->backendDisconnect();
            }
        };

        serverHandle->visitSessions(visitor);
    }
    else if (subcommand == "PRINT") {
        d_vhostState_p->print(output);
    }
    else {
        output << "Subcommand not recognised.\n";
    }
}

}
}
