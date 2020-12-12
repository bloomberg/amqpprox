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
#include <amqpprox_loggingcontrolcommand.h>

#include <amqpprox_logging.h>

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <string>

namespace Bloomberg {
namespace amqpprox {

LoggingControlCommand::LoggingControlCommand()
{
}

std::string LoggingControlCommand::commandVerb() const
{
    return "LOG";
}

std::string LoggingControlCommand::helpText() const
{
    return "CONSOLE verbosity | FILE verbosity";
}

void LoggingControlCommand::handleCommand(const std::string & /* command */,
                                          const std::string &  restOfCommand,
                                          const OutputFunctor &outputFunctor,
                                          Server * /* serverHandle */,
                                          Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    int                verbosity;

    if (iss >> subcommand && iss >> verbosity) {
        boost::to_upper(subcommand);

        if (subcommand == "FILE") {
            Logging::setFileVerbosity(verbosity);
        }
        else if (subcommand == "CONSOLE") {
            Logging::setConsoleVerbosity(verbosity);
        }
        else {
            output << "Type to verb is not known.\n";
            return;
        }
    }
    else {
        output << "Command or verbosity not found.\n";
    }
}

}
}
