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

#include <amqpprox_maybesecuresocketadaptor.h>
#include <amqpprox_simpleconnectionselector.h>

#include <boost/asio.hpp>

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

class Server {
    using SessionPtr = std::shared_ptr<Session>;

    boost::asio::io_service                  d_ioService;
    boost::asio::ssl::context                d_ingressTlsContext;
    boost::asio::ssl::context                d_egressTlsContext;
    MaybeSecureSocketAdaptor                 d_socket;
    boost::asio::deadline_timer              d_timer;
    std::unordered_map<uint64_t, SessionPtr> d_sessions;
    std::unordered_set<SessionPtr>           d_deletingSessions;
    std::unordered_map<int, boost::asio::ip::tcp::acceptor> d_listeningSockets;
    ConnectionSelector *d_connectionSelector_p;
    // HELD NOT OWNED
    EventSource *d_eventSource_p;
    // HELD NOT OWNED
    BufferPool *d_bufferPool_p;
    // HELD NOT OWNED
    std::mutex                      d_mutex;
    std::shared_ptr<HostnameMapper> d_hostnameMapper;
    std::string d_localHostname;

  public:
    Server(ConnectionSelector *selector,
           EventSource *       eventSource,
           BufferPool *        bufferPool);

    ~Server();

    // MANIPULATORS

    int run();
    ///< Run the server event loop

    void stop();
    ///< Stop the server event loop

    void startListening(int port, bool secure);
    ///< Start listening on the given port, no op if the server is already
    ///< listening on the specified port.

    void stopListening(int port);
    ///< Stop listening on the server's port, no op if the server is not
    ///< already listening.

    void stopAllListening();
    ///< Stop listening on all the server's ports, no op if the server
    ///< is not already listening.

    void removeSession(uint64_t identifier);
    ///< Remove the session given by its identifier

    void clearDefunctSessions();
    ///< Clear defunct sessions

    void
    setHostnameMapper(const std::shared_ptr<HostnameMapper> &hostnameMapper);
    ///< Set a different hostname mapper.  Note that this does not
    ///< update any hostname mappers that are currently set on any
    ///< sessions.

    // ACCESSORS

    std::shared_ptr<Session> getSession(uint64_t identifier);

    void visitSessions(const std::function<void(const SessionPtr &)> &visitor);

    void printConnections(std::ostream &os);

    boost::asio::ssl::context &ingressTlsContext();

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
