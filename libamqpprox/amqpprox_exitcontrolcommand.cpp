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
#include <amqpprox_exitcontrolcommand.h>

#include <amqpprox_control.h>
#include <amqpprox_server.h>

namespace Bloomberg {
namespace amqpprox {

std::string ExitControlCommand::commandVerb() const
{
    return "EXIT";
}

std::string ExitControlCommand::helpText() const
{
    return "Exit the program gracefully.";
}

void ExitControlCommand::handleCommand(const std::string & /* command */,
                                       const std::string & /* restOfCommand */,
                                       const OutputFunctor &outputFunctor,
                                       Server              *serverHandle,
                                       Control             *controlHandle)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    output << "Exiting.\n";
    serverHandle->stop();
    controlHandle->stop();
}

}
}
