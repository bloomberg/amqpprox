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
#include <amqpprox_control.h>

#include <amqpprox_controlcommand.h>
#include <amqpprox_logging.h>
#include <amqpprox_server.h>

#include <boost/algorithm/string.hpp>

#include <iostream>

using boost::asio::local::stream_protocol;
using namespace boost::system;

namespace Bloomberg {
namespace amqpprox {

/* \brief Helper class for encapsulating a UNIX Domain Socket connection for
 * the control channel.
 *
 * This component holds open a socket between the amqpprox_ctl program and the
 * `Control` component. It encapsulates reading the input from the socket,
 * which is then dispatched into the appropriate `ControlCommand`. The
 * lifecycle of individual `ControlSession` objects is usually bound to the
 * lifetime of the `outputFunc` that is passed into the `ControlCommand`'s
 * `handleCommand` function. That functor specifies if the `ControlCommand` has
 * finished its output. If the `ControlCommand` holds onto the reference, for
 * example to stream many items out, it is notified by the return code to throw
 * away its reference the next time the functor is called after the connection
 * is finished. Being finished is controlled by the `d_finished` flag and can
 * be set either by a previous call which indicated the output was finished, or
 * by a socket error.
 */
class ControlSession : public std::enable_shared_from_this<ControlSession> {
    stream_protocol::socket d_socket;
    Server *                d_server_p;
    Control *               d_control_p;
    EventSource *           d_eventSource_p;
    boost::asio::streambuf  d_streamBuf;
    bool                    d_finished;

  public:
    ControlSession(stream_protocol::socket socket,
                   Server *                server,
                   Control *               control,
                   EventSource *           eventSource)
    : d_socket(std::move(socket))
    , d_server_p(server)
    , d_control_p(control)
    , d_eventSource_p(eventSource)
    , d_streamBuf()
    , d_finished(false)
    {
    }

    ~ControlSession() {}

    void processInput(const std::string &input)
    {
        auto spaceLocation = input.find_first_of(' ');

        std::string remaining{""};
        if (spaceLocation != std::string::npos) {
            remaining = input.substr(spaceLocation);
        }

        auto commandVerb = input.substr(0, spaceLocation);
        boost::to_upper(commandVerb);

        auto self(shared_from_this());
        auto outputFunc = [this, self, input](const std::string &output,
                                              bool finish) -> bool {
            LOG_INFO << "Control '" << input << "' Output (finish=" << finish
                     << "): " << output;
            return sendOutput(output, finish);
        };

        auto controlCommand = d_control_p->getControlCommand(commandVerb);
        if (controlCommand) {
            controlCommand->handleCommand(
                commandVerb, remaining, outputFunc, d_server_p, d_control_p);
        }
        else {
            auto helpCommand = d_control_p->getControlCommand("HELP");
            helpCommand->handleCommand(
                "HELP", "", outputFunc, d_server_p, d_control_p);
        }
    }

    bool sendOutput(const std::string &output, bool finishOutput)
    {
        if (d_finished) {
            return false;
        }

        auto self(shared_from_this());
        auto spOutput = std::make_shared<std::string>(output);
        boost::asio::async_write(
            d_socket,
            boost::asio::buffer(spOutput->data(), spOutput->length()),
            [this, finishOutput, self, spOutput](error_code  ec,
                                                 std::size_t length) {
                if (ec) {
                    d_finished = true;
                    return;
                }

                if (finishOutput) {
                    d_finished = true;
                    error_code shutdownErr;
                    d_socket.shutdown(stream_protocol::socket::shutdown_both,
                                      shutdownErr);
                    d_socket.close();
                }
            });

        return true;
    }

    void start() { readInput(); }

    void readInput()
    {
        auto self(shared_from_this());
        boost::asio::async_read_until(
            d_socket,
            d_streamBuf,
            '\n',
            [this, self](error_code ec, std::size_t length) {
                if (!ec) {
                    std::istream is(&d_streamBuf);
                    std::string  line;
                    std::getline(is, line);
                    processInput(line);
                    readInput();
                }
                else {
                    d_finished = true;
                }
            });
    }
};

Control::Control(Server *           server,
                 EventSource *      source,
                 const std::string &udsPath)
: d_server_p(server)
, d_eventSource_p(source)
, d_ioService()
, d_acceptor(d_ioService, stream_protocol::endpoint(udsPath))
, d_socket(d_ioService)
{
    boost::asio::socket_base::reuse_address option(true);
    d_acceptor.set_option(option);

    doAccept();
}

void Control::run()
{
    try {
        d_ioService.run();
    }
    catch (std::exception &e) {
        LOG_FATAL << "Control Thread Exception: " << e.what();
    }
}

void Control::stop()
{
    d_ioService.stop();
}

void Control::scheduleRecurringEvent(
    int                                             intervalMs,
    const std::string &                             name,
    const std::function<bool(Control *, Server *)> &event)
{
    auto duration = boost::posix_time::milliseconds(intervalMs);
    auto timer    = std::make_shared<boost::asio::deadline_timer>(d_ioService);
    timer->expires_from_now(duration);
    timer->async_wait([this, name, intervalMs, event, timer](
                          const boost::system::error_code &ec) {
        if (ec) {
            LOG_FATAL << "Error from recurring event: " << ec;
            return;
        }

        if (event(this, d_server_p)) {
            scheduleRecurringEvent(intervalMs, name, event);
        }
        else {
            LOG_DEBUG << "Recurring event '" << name << "' requested to stop";
        }
    });
}

void Control::addControlCommand(std::unique_ptr<ControlCommand> command)
{
    std::string verb        = command->commandVerb();
    d_controlCommands[verb] = std::move(command);
}

ControlCommand *Control::getControlCommand(const std::string &verb)
{
    auto it = d_controlCommands.find(verb);
    if (it != d_controlCommands.end()) {
        return it->second.get();
    }
    else {
        return nullptr;
    }
}

void Control::visitControlCommands(
    const std::function<void(ControlCommand *)> &visitor)
{
    for (const auto &command : d_controlCommands) {
        visitor(command.second.get());
    }
}

void Control::doAccept()
{
    d_acceptor.async_accept(d_socket, [this](error_code ec) {
        if (!ec) {
            auto controlSession = std::make_shared<ControlSession>(
                std::move(d_socket), d_server_p, this, d_eventSource_p);
            controlSession->start();
        }

        doAccept();
    });
}

boost::asio::io_service &Control::ioService()
{
    return d_ioService;
}

}
}
