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
#ifndef BLOOMBERG_AMQPPROX_LIMITCONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_LIMITCONTROLCOMMAND

#include <amqpprox_control.h>
#include <amqpprox_controlcommand.h>
#include <amqpprox_server.h>

#include <string>

namespace Bloomberg {
namespace amqpprox {

class ConnectionLimiterManager;

class LimitControlCommand : public ControlCommand {
    ConnectionLimiterManager *d_connectionLimiterManager_p;  // HELD NOT OWNED

  public:
    // CREATORS
    explicit LimitControlCommand(
        ConnectionLimiterManager *connectionLimiterManager);

    virtual ~LimitControlCommand() override = default;

    // MANIPULATORS
    /**
     * \return the command verb this handle
     */
    virtual std::string commandVerb() const override;

    /**
     * \return a string of the help text for this command
     */
    virtual std::string helpText() const override;

    /**
     * \brief Execute a command, providing any output to the provided functor
     * \param command to execute
     * \param restOfCommand parameters for the command
     * \param outputFunctor is called back with the output
     * \param serverHandle access to the Server object
     * \param controlHandle access to the Control object
     */
    virtual void handleCommand(const std::string   &command,
                               const std::string   &restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server              *serverHandle,
                               Control             *controlHandle) override;
};

}
}

#endif
