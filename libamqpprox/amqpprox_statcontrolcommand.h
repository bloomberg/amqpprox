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
#ifndef BLOOMBERG_AMQPPROX_STATCONTROLCOMMAND
#define BLOOMBERG_AMQPPROX_STATCONTROLCOMMAND

#include <amqpprox_controlcommand.h>
#include <amqpprox_eventsourcesignal.h>
#include <amqpprox_statcollector.h>

#include <list>
#include <string>
#include <utility>

namespace Bloomberg {
namespace amqpprox {

class StatSnapshot;
class EventSource;

class StatControlCommand : public ControlCommand {
    using StatFunctor = std::function<bool(const StatSnapshot &)>;

    std::list<std::pair<StatFunctor, bool>> d_functors;
    EventSubscriptionHandle                 d_statisticsAvailableSignal;
    EventSource *                           d_eventSource_p;  // HELD NOT OWNED

  public:
    explicit StatControlCommand(EventSource *eventSource);

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

  private:
    void invokeHandlers(StatCollector *collector);
    ///< Invoke the stats on all of the functors waiting for information
};

}
}

#endif
