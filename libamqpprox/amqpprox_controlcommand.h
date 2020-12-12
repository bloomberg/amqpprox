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

class ControlCommand {
  public:
    using OutputFunctor = std::function<bool(const std::string &, bool)>;

    // CREATORS
    virtual ~ControlCommand() = default;

    // MANIPULATORS
    virtual void handleCommand(const std::string &  command,
                               const std::string &  restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server *             serverHandle,
                               Control *            controlHandle) = 0;
    ///< Execute a command, providing any output to the provided functor

    // ACCESSORS
    virtual std::string commandVerb() const = 0;
    ///< Returns the command verb this handles

    virtual std::string helpText() const = 0;
    ///< Returns a string of the help text for this command
};

}
}

#endif
