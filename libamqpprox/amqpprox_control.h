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
#ifndef BLOOMBERG_AMQPPROX_CONTROL
#define BLOOMBERG_AMQPPROX_CONTROL

#include <amqpprox_controlcommand.h>

#include <boost/asio.hpp>

#include <map>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class EventSource;
class Server;

class Control {
    Server *                                               d_server_p;
    EventSource *                                          d_eventSource_p;
    boost::asio::io_service                                d_ioService;
    boost::asio::local::stream_protocol::acceptor          d_acceptor;
    boost::asio::local::stream_protocol::socket            d_socket;
    std::map<std::string, std::unique_ptr<ControlCommand>> d_controlCommands;

  public:
    // CREATORS
    Control(Server *server, EventSource *source, const std::string &udsPath);

    // MANIPULATORS
    void run();
    ///< Start the control thread running, blocking call until it is stopped

    void stop();
    ///< Stop the control thread gracefully

    void scheduleRecurringEvent(
        int                                             intervalMs,
        const std::string &                             name,
        const std::function<bool(Control *, Server *)> &event);
    ///< Schedule an event identified by `name` for `intervalMs`
    ///< milliseconds later, and if the called event functor returns true,
    ///< it will be scheduled again.

    void addControlCommand(std::unique_ptr<ControlCommand> command);
    ///< Register a control command handling object

    // ACCESSORS
    ControlCommand *getControlCommand(const std::string &verb);
    ///< Retrieve a control command handler by its command verb

    void
    visitControlCommands(const std::function<void(ControlCommand *)> &visitor);
    ///< Visit all of the control command handlers in an unspecified order

    boost::asio::io_service &ioService();
    ///< Retrieve the io service for the control thread

  private:
    void doAccept();
};

}
}

#endif
