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
#ifndef BLOOMBERG_AMQPPROX_CONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_CONTROLCOMMAND

#include <functional>
#include <sstream>

namespace Bloomberg {
namespace amqpprox {

class Control;
class Server;

/**
 * \brief Represents control command output
 */
template <typename Functor>
class ControlCommandOutput : public std::ostringstream {
  private:
    Functor d_functor;

  public:
    explicit ControlCommandOutput(const Functor &functor)
    : d_functor(functor)
    {
    }

    virtual ~ControlCommandOutput() override { d_functor(this->str(), true); }
};

/**
 * \brief Class for executing a control command
 */
class ControlCommand {
  public:
    using OutputFunctor = std::function<bool(const std::string &, bool)>;

    // CREATORS
    virtual ~ControlCommand() = default;

    // MANIPULATORS
    /**
     * \brief Execute a command, providing any output to the provided functor
     * \param command Command
     * \param restOfCommand Rest of command
     * \param outputFunctor Output functor
     * \param serverHandle Server handle
     * \param controlHandle Control handle
     */
    virtual void handleCommand(const std::string   &command,
                               const std::string   &restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server              *serverHandle,
                               Control             *controlHandle) = 0;

    // ACCESSORS
    /**
     * \return Command verb this handles
     */
    virtual std::string commandVerb() const = 0;

    /**
     * \return Help text for this command
     */
    virtual std::string helpText() const = 0;
};

}
}

#endif
