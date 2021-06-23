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
#ifndef BLOOMBERG_AMQPPROX_FARMCONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_FARMCONTROLCOMMAND

#include <amqpprox_controlcommand.h>

namespace Bloomberg {
namespace amqpprox {

class BackendSelectorStore;
class BackendStore;
class FarmStore;
class PartitionPolicyStore;

class FarmControlCommand : public ControlCommand {
  private:
    // DATA
    FarmStore *           d_store_p;                 // HELD NOT OWNED
    BackendStore *        d_backendStore_p;          // HELD NOT OWNED
    BackendSelectorStore *d_backendSelectorStore_p;  // HELD NOT OWNED
    PartitionPolicyStore *d_partitionPolicyStore_p;  // HELD NOT OWNED

  public:
    // CREATORS
    FarmControlCommand(FarmStore *           store,
                       BackendStore *        backendStore,
                       BackendSelectorStore *backendSelectorStore,
                       PartitionPolicyStore *partitionPolicyStore);

    virtual ~FarmControlCommand() override = default;

    // MANIPULATORS
    virtual void handleCommand(const std::string &  command,
                               const std::string &  restOfCommand,
                               const OutputFunctor &outputFunctor,
                               Server *             serverHandle,
                               Control *            controlHandle) override;
    ///< Execute a command, providing any output to the provided functor

    // ACCESSORS
    virtual std::string commandVerb() const override;
    ///< Returns the command verb this handles

    virtual std::string helpText() const override;
    ///< Returns a string of the help text for this command
};

}
}

#endif
