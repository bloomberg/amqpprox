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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONSCONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_CONNECTIONSCONTROLCOMMAND

#include <amqpprox_controlcommand.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Represents a connections control command
 */
class ConnectionsControlCommand : public ControlCommand {
  public:
    /**
     * \return Command verb this handles
     */
    virtual std::string commandVerb() const override;

    /**
     * \return String of the help text for this command
     */
    virtual std::string helpText() const override;

    /**
     * \brief Execute a command, providing any output to the provided functor
     * \param command Command
     * \param restOfCommand Rest of command
     * \param outputFunctor Outpuf functor
     * \param serverHandle Server handle
     * \param controlHandle Control handle
     */
    virtual void handleCommand(const std::string &  command,
                               const std::string &  restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server *             serverHandle,
                               Control *            controlHandle) override;
};

}
}

#endif
