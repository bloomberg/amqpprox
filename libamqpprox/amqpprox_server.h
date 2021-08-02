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
#ifndef BLOOMBERG_AMQPPROX_SERVER
#define BLOOMBERG_AMQPPROX_SERVER

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_connectionselector.h>
#include <amqpprox_dnsresolver.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>

#include <iosfwd>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace Bloomberg {
namespace amqpprox {

class BufferPool;
class Session;
class EventSource;
class HostnameMapper;

/**
 * \brief Sockets are accept()'ed in this component. For each incoming
 * connection, it creates a Session object, and stores it in a threadsafe
 * collection. It is the primary thread, where event loop runs.
 */
class Server {
    using SessionPtr = std::shared_ptr<Session>;

    boost::asio::io_service                  d_ioService;
    boost::asio::ssl::context                d_ingressTlsContext;
    boost::asio::ssl::context                d_egressTlsContext;
    boost::asio::deadline_timer              d_timer;
    std::unordered_map<uint64_t, SessionPtr> d_sessions;
    std::unordered_set<SessionPtr>           d_deletingSessions;
    std::unordered_map<int, boost::asio::ip::tcp::acceptor> d_listeningSockets;
    DNSResolver                                             d_dnsResolver;
    ConnectionSelector *            d_connectionSelector_p;  // HELD NOT OWNED
    EventSource *                   d_eventSource_p;         // HELD NOT OWNED
    BufferPool *                    d_bufferPool_p;          // HELD NOT OWNED
    std::mutex                      d_mutex;
    std::shared_ptr<HostnameMapper> d_hostnameMapper;
    std::string                     d_localHostname;
    std::shared_ptr<AuthInterceptInterface> d_authIntercept;

  public:
    Server(ConnectionSelector *selector,
           EventSource *       eventSource,
           BufferPool *        bufferPool);

    ~Server();

    // MANIPULATORS
    /**
     * \brief Run the server event loop
     */
    int run();

    /**
     * \brief Stop the server event loop
     */
    void stop();

    /**
     * \brief Start listening on the given port, no op if the server is already
     * listening on the specified port.
     * \param port to start listening on
     * \param secure to set the socket into secure (using TLS) or unsecure
     * (plaintext)
     */
    void startListening(int port, bool secure);

    /**
     * \brief Stop listening on the server's port, no op if the server is not
     * already listening.
     * \param port to stop listening on
     */
    void stopListening(int port);

    /**
     * \brief Stop listening on all the server's ports, no op if the server is
     * not already listening. already listening.
     */
    void stopAllListening();

    /**
     * \brief Remove the session given by its identifier
     * \param identifier a unique session identifier
     */
    void removeSession(uint64_t identifier);

    /**
     * \brief Clear defunct sessions
     */
    void clearDefunctSessions();

    /**
     * \brief Set a different hostname mapper. Note that this does not update
     * any hostname mappers that are currently set on any sessions.
     * \param hostnameMapper to facilitate reverse lookups based on IP address
     * to hostname
     */
    void
    setHostnameMapper(const std::shared_ptr<HostnameMapper> &hostnameMapper);

    // ACCESSORS
    /**
     * \brief Get a particular session for a specified ID
     * \param identifier a unique session identifier
     */
    std::shared_ptr<Session> getSession(uint64_t identifier);

    /**
     * \brief Visit all the sessions
     * \param visitor callback function to be invoked with session shared
     * pointer as an argument for each session
     */
    void visitSessions(const std::function<void(const SessionPtr &)> &visitor);

    /**
     * \brief Print all the connections information for all the sessions
     * \param os output stream object
     */
    void printConnections(std::ostream &os);

    /**
     * \brief Return ingress (server) TLS context
     */
    boost::asio::ssl::context &ingressTlsContext();

    /**
     * \brief Return egress (client) TLS context
     */
    boost::asio::ssl::context &egressTlsContext();

  private:
    void doAccept(int port, bool secure);
    void doTimer();
    void timer();
    void closeListeners();
};

}
}

#endif
