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
#include <amqpprox_server.h>

#include <amqpprox_bufferpool.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_hostnamemapper.h>
#include <amqpprox_logging.h>
#include <amqpprox_session.h>
#include <amqpprox_tlsutil.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

using namespace boost::asio::ip;
using namespace boost::system;

Server::Server(ConnectionSelector *selector,
               EventSource *       eventSource,
               BufferPool *        bufferPool)
: d_ioService()
, d_ingressTlsContext(boost::asio::ssl::context::tlsv12)
, d_egressTlsContext(boost::asio::ssl::context::tlsv12)
, d_socket(d_ioService, d_ingressTlsContext, false)
, d_timer(d_ioService, boost::posix_time::time_duration(0, 0, 10, 0))
, d_sessions()
, d_deletingSessions()
, d_listeningSockets()
, d_dnsResolver(d_ioService)
, d_connectionSelector_p(selector)
, d_eventSource_p(eventSource)
, d_bufferPool_p(bufferPool)
, d_mutex()
, d_hostnameMapper()
, d_localHostname(boost::asio::ip::host_name())
{
    d_ingressTlsContext.set_options(
        boost::asio::ssl::context::default_workarounds);
    d_egressTlsContext.set_options(
        boost::asio::ssl::context::default_workarounds);

    TlsUtil::setupTlsLogging(d_ingressTlsContext);
    TlsUtil::setupTlsLogging(d_egressTlsContext);

    timer();
}

Server::~Server()
{
    closeListeners();

    // Ensure the io_service is stopped fully
    if (!d_ioService.stopped()) {
        d_ioService.stop();
    }
}

int Server::run()
{
    try {
        d_ioService.run();
    }
    catch (std::exception &e) {
        LOG_FATAL << "Exception: " << e.what();
        closeListeners();

        return 1;
    }

    return 0;
}

void Server::closeListeners()
{
    for (auto &p : d_listeningSockets) {
        p.second.close();
    }
}

void Server::stop()
{
    d_ioService.stop();
}

void Server::startListening(int port, bool secure)
{
    d_ioService.dispatch([this, port, secure] {
        {
            std::lock_guard<std::mutex> lg(d_mutex);
            auto                        it = d_listeningSockets.find(port);
            if (it != d_listeningSockets.end()) {
                LOG_ERROR << "Already listening on port: " << port;
                return;
            }

            boost::asio::ip::tcp::acceptor acceptor(d_ioService);
            tcp::endpoint                  local_endpoint(tcp::v4(), port);
            acceptor.open(local_endpoint.protocol());
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));
            acceptor.bind(local_endpoint);
            acceptor.listen(boost::asio::socket_base::max_connections);
            d_listeningSockets.emplace(port, std::move(acceptor));
        }

        doAccept(port, secure);
    });
}

void Server::stopListening(int port)
{
    d_ioService.dispatch([this, port] {
        std::lock_guard<std::mutex> lg(d_mutex);
        auto                        it = d_listeningSockets.find(port);
        if (it != d_listeningSockets.end()) {
            boost::system::error_code ec;
            it->second.close(ec);
            d_listeningSockets.erase(it);
            if (ec) {
                LOG_WARN << "Closing listening socket failed: "
                         << ec.message();
            }
        }
    });
}

void Server::stopAllListening()
{
    std::lock_guard<std::mutex> lg(d_mutex);
    for (auto &p : d_listeningSockets) {
        stopListening(p.first);
    }
}

void Server::timer()
{
    d_timer.expires_from_now(boost::posix_time::time_duration(0, 0, 10, 0));
    d_timer.async_wait([this](const boost::system::error_code &) {
        doTimer();
        timer();
    });
}

void Server::doTimer()
{
    // Currently empty, the timer prevents the service shutting down when not
    // listening.
}

void Server::doAccept(int port, bool secure)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    auto                        it = d_listeningSockets.find(port);
    if (it == d_listeningSockets.end()) {
        return;
    }

    d_socket.refreshSocketContext();
    it->second.async_accept(
        d_socket.socket(), [this, port, secure](error_code ec) {
            if (!ec) {
                d_socket.setSecure(secure);

                MaybeSecureSocketAdaptor clientSocket(
                    d_ioService, d_egressTlsContext, false);

                auto session =
                    std::make_shared<Session>(d_ioService,
                                              std::move(d_socket),
                                              std::move(clientSocket),
                                              d_connectionSelector_p,
                                              d_eventSource_p,
                                              d_bufferPool_p,
                                              &d_dnsResolver,
                                              d_hostnameMapper,
                                              d_localHostname);

                {
                    std::lock_guard<std::mutex> lg(d_mutex);
                    d_sessions[session->state().id()] = session;
                }

                session->start();
                d_eventSource_p->connectionReceived().emit(
                    session->state().id());
            }
            else {
                LOG_ERROR << "Accept failed, reason: " << ec.message();
                // When the accept fails, we still fall through to try the
                // accept again.
            }

            doAccept(port, secure);
        });
}

void Server::printConnections(std::ostream &os)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    for (const auto &conn : d_sessions) {
        conn.second->print(os);
    }
}

void Server::clearDefunctSessions()
{
    std::lock_guard<std::mutex>    lg(d_mutex);
    std::unordered_set<SessionPtr> deletionCopy(d_deletingSessions);
    d_deletingSessions.clear();

    d_ioService.post([this, deletionCopy]() mutable {
        // Here we **post** onto the io loop so as to avoid recursively locking
        // the mutex. Although the mutex guards no data from this mutable
        // lambda, we use it to ensure the posted event does not occur before
        // the destructors of the clearDefunctSessions method have occurred in
        // the posting thread.
        std::lock_guard<std::mutex> lg(d_mutex);
        deletionCopy.clear();
    });
}

void Server::setHostnameMapper(
    const std::shared_ptr<HostnameMapper> &hostnameMapper)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_hostnameMapper = hostnameMapper;
}

std::shared_ptr<Session> Server::getSession(uint64_t identifier)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    auto                        s = d_sessions.find(identifier);
    if (s == d_sessions.end()) {
        return std::shared_ptr<Session>();
    }
    return s->second;
}

void Server::removeSession(uint64_t identifier)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    auto                        session = d_sessions.find(identifier);
    if (session != d_sessions.end()) {
        // Here we take a copy of the session being removed immediately and
        // erase from the live session map. This ensures all subsequent calls
        // that may visit all live sessions do not retain a reference to the
        // deleted sessions.
        d_deletingSessions.insert(session->second);
        d_sessions.erase(identifier);
    }
}

void Server::visitSessions(
    const std::function<void(const SessionPtr &)> &visitor)
{
    std::unordered_map<uint64_t, SessionPtr> sessions;

    {
        std::lock_guard<std::mutex> lg(d_mutex);
        sessions = std::unordered_map<uint64_t, SessionPtr>(d_sessions);
    }

    for (const auto &conn : sessions) {
        visitor(conn.second);
    }
}

boost::asio::ssl::context &Server::ingressTlsContext()
{
    return d_ingressTlsContext;
}

boost::asio::ssl::context &Server::egressTlsContext()
{
    return d_egressTlsContext;
}

}  // namespace amqpprox
}  // namespace Bloomberg
