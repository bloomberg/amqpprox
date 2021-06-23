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
#ifndef BLOOMBERG_AMQPPROX_MAPCONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_MAPCONTROLCOMMAND

#include <amqpprox_controlcommand.h>

namespace Bloomberg {
namespace amqpprox {

class ResourceMapper;
class MappingConnectionSelector;

class MapControlCommand : public ControlCommand {
    ResourceMapper *           d_mapper_p;    // HELD NOT OWNED
    MappingConnectionSelector *d_selector_p;  // HELD NOT OWNED

  public:
    MapControlCommand(ResourceMapper *           mapper,
                      MappingConnectionSelector *selector);

    virtual std::string commandVerb() const override;
    ///< Returns the command verb this handles

    virtual std::string helpText() const override;
    ///< Returns a string of the help text for this command

    virtual void handleCommand(const std::string &  command,
                               const std::string &  restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server *             serverHandle,
                               Control *            controlHandle) override;
    ///< Execute a command, providing any output to the provided functor
};

}
}

#endif
