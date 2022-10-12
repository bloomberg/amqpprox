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
#include <amqpprox_connectionselectorinterface.h>
#include <amqpprox_defaultauthintercept.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_hostnamemapper.h>
#include <amqpprox_logging.h>
#include <amqpprox_maybesecuresocketadaptor.h>
#include <amqpprox_session.h>
#include <amqpprox_tlsutil.h>

#include <boost/asio/ssl/context.hpp>
#include <openssl/opensslv.h>

#include <iostream>
#include <memory>

namespace Bloomberg {
namespace amqpprox {

using namespace boost::asio::ip;
using namespace boost::system;

void initTLS(boost::asio::ssl::context &context)
{
    context.set_options(boost::asio::ssl::context::default_workarounds);

    // Custom TLS flags
    SSL_CTX *sslContext = context.native_handle();

    long flags = SSL_CTX_get_options(sslContext);

    flags |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    SSL_CTX_set_options(sslContext, flags);

    // Explicitly load ECC ciphers
    // SSL_CTX_set_ecdh_auto is deprecated and removed in OpenSSL 1.1.x -
    // https://github.com/openssl/openssl/issues/1437
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_CTX_set_ecdh_auto(sslContext, 1);
#endif  // OPENSSL_VERSION_NUMBER < 0x10100000L
    TlsUtil::setupTlsLogging(context);
}

Server::Server(ConnectionSelectorInterface *selector,
               EventSource                 *eventSource,
               BufferPool                  *bufferPool,
               DataRateLimitManager        *limitManager)
: d_ioContext()
, d_ingressTlsContext(boost::asio::ssl::context::tlsv12)
, d_egressTlsContext(boost::asio::ssl::context::tlsv12)
, d_timer(d_ioContext, boost::posix_time::time_duration(0, 0, 10, 0))
, d_sessions()
, d_deletingSessions()
, d_listeningSockets()
, d_dnsResolver(d_ioContext)
, d_connectionSelector_p(selector)
, d_eventSource_p(eventSource)
, d_bufferPool_p(bufferPool)
, d_mutex()
, d_hostnameMapper()
, d_localHostname(boost::asio::ip::host_name())
, d_authIntercept(std::make_shared<DefaultAuthIntercept>(d_ioContext))
, d_limitManager(limitManager)
{
    d_dnsResolver.setCacheTimeout(1000);
    d_dnsResolver.startCleanupTimer();

    initTLS(d_ingressTlsContext);
    initTLS(d_egressTlsContext);

    timer();
}

Server::~Server()
{
    d_dnsResolver.stopCleanupTimer();
    closeListeners();

    // Ensure the io_context is stopped fully
    if (!d_ioContext.stopped()) {
        d_ioContext.stop();
    }
}

int Server::run()
{
    try {
        d_ioContext.run();
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
    d_ioContext.stop();
}

void Server::startListening(int port, bool secure)
{
    d_ioContext.dispatch([this, port, secure] {
        {
            std::lock_guard<std::mutex> lg(d_mutex);
            auto                        it = d_listeningSockets.find(port);
            if (it != d_listeningSockets.end()) {
                LOG_ERROR << "Already listening on port: " << port;
                return;
            }

            boost::asio::ip::tcp::acceptor acceptor(d_ioContext);
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
    d_ioContext.dispatch([this, port] {
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

    std::shared_ptr<MaybeSecureSocketAdaptor<>> incomingSocket =
        std::make_shared<MaybeSecureSocketAdaptor<>>(
            d_ioContext, d_ingressTlsContext, secure);

    it->second.async_accept(
        incomingSocket->socket(),
        [this, port, secure, incomingSocket](error_code ec) {
            if (!ec) {
                std::shared_ptr<MaybeSecureSocketAdaptor<>> clientSocket =
                    std::make_shared<MaybeSecureSocketAdaptor<>>(
                        d_ioContext, d_egressTlsContext, false);

                auto session =
                    std::make_shared<Session>(d_ioContext,
                                              incomingSocket,
                                              clientSocket,
                                              d_connectionSelector_p,
                                              d_eventSource_p,
                                              d_bufferPool_p,
                                              &d_dnsResolver,
                                              d_hostnameMapper,
                                              d_localHostname,
                                              d_authIntercept,
                                              secure,
                                              d_limitManager);

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

    d_ioContext.post([this, deletionCopy]() mutable {
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

void Server::setAuthIntercept(
    const std::shared_ptr<AuthInterceptInterface> &authIntercept)
{
    std::lock_guard<std::mutex> lg(d_mutex);
    d_authIntercept = authIntercept;
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

boost::asio::io_context &Server::ioContext()
{
    return d_ioContext;
}

std::shared_ptr<AuthInterceptInterface> Server::getAuthIntercept() const
{
    return d_authIntercept;
}

DNSResolver *Server::getDNSResolverPtr()
{
    return &d_dnsResolver;
}

}  // namespace amqpprox
}  // namespace Bloomberg
