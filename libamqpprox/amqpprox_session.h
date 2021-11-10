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
#ifndef BLOOMBERG_AMQPPROX_SESSION
#define BLOOMBERG_AMQPPROX_SESSION

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_backend.h>
#include <amqpprox_buffer.h>
#include <amqpprox_bufferhandle.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_connector.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_flowtype.h>
#include <amqpprox_frame.h>
#include <amqpprox_maybesecuresocketadaptor.h>
#include <amqpprox_sessionstate.h>

#include <boost/asio.hpp>

#include <chrono>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class ConnectionManager;
class ConnectionSelector;
class EventSource;
class DNSResolver;

/**
 * \brief Binds the incoming and outgoing sockets into a channel through the
 * proxy.
 *
 * This component provides the 'pumping' of data through the proxy for a
 * particular session between one incoming connection and one outgoing
 * connection (once the handshake has completed).
 */
class Session : public std::enable_shared_from_this<Session> {
  private:
    using TimePoint =
        std::chrono::time_point<std::chrono::high_resolution_clock>;

    boost::asio::io_service &d_ioService;
    MaybeSecureSocketAdaptor d_serverSocket;
    MaybeSecureSocketAdaptor d_clientSocket;
    BufferHandle             d_serverDataHandle;
    BufferHandle             d_serverWriteDataHandle;
    BufferHandle             d_clientDataHandle;
    BufferHandle             d_clientWriteDataHandle;
    std::size_t              d_serverWaterMark;
    std::size_t              d_clientWaterMark;
    SessionState             d_sessionState;
    Connector                d_connector;
    ConnectionSelector *     d_connectionSelector_p;  // HELD NOT OWNED
    EventSource *            d_eventSource_p;         // HELD NOT OWNED
    BufferPool *             d_bufferPool_p;          // HELD NOT OWNED
    DNSResolver *            d_dnsResolver_p;
    TimePoint                d_ingressWaitingSince;
    TimePoint                d_egressWaitingSince;
    uint32_t                 d_egressRetryCounter;
    bool                     d_ingressCurrentlyReading;
    TimePoint                d_ingressStartedAt;
    bool                     d_egressCurrentlyReading;
    TimePoint                d_egressStartedAt;
    std::vector<boost::asio::ip::tcp::endpoint> d_resolvedEndpoints;
    uint32_t                                    d_resolvedEndpointsIndex;
    std::shared_ptr<AuthInterceptInterface>     d_authIntercept;

  public:
    // CREATORS
    Session(boost::asio::io_service &                      ioservice,
            MaybeSecureSocketAdaptor &&                    serverSocket,
            MaybeSecureSocketAdaptor &&                    clientSocket,
            ConnectionSelector *                           connectionSelector,
            EventSource *                                  eventSource,
            BufferPool *                                   bufferPool,
            DNSResolver *                                  dnsResolver,
            const std::shared_ptr<HostnameMapper> &        hostnameMapper,
            std::string_view                               localHostname,
            const std::shared_ptr<AuthInterceptInterface> &authIntercept);

    ~Session();

    // MANIPULATORS
    /**
     * \brief Start the session and read the new socket for the protocol header
     */
    void start();

    /**
     * \brief Print the session information
     * \param os output stream object
     */
    void print(std::ostream &os);

    /**
     * \brief Pause all IO operations on the session
     */
    void pause();

    /**
     * \brief Disconnect both sides of the session
     * \param forcible to specify forcefully disconnect
     */
    void disconnect(bool forcible);

    /**
     * \brief Disconnect unauthorized client side connection, if client sets
     * 'authentication_failure_close' capability to true in AMQP START-OK
     * connection method; otherwise snap the socket for client side connection.
     * And snap the socket for server side connection.
     * https://www.rabbitmq.com/auth-notification.html
     * \param clientProperties will be used to find the client
     * 'authentication_failure_close' capability value
     */
    void disconnectUnauthClient(const FieldTable &clientProperties);

    /**
     * \brief Disconnect the session from the backend, but leave the client
     * connected.
     */
    void backendDisconnect();

    /**
     * \return the state object for the session
     */
    SessionState &state();

    // ACCESSORS
    /**
     * \return the threadsafe state object for the session
     */
    const SessionState &state() const;

    /**
     * \return true if the session is disconnected and should be
     * elligible to be deleted.
     */
    bool finished();

    /**
     * \brief Give proxy protocol header for the given backend
     * \param currentBackend pointer to `Backend`
     * \return proxy protocol header
     */
    std::string getProxyProtocolHeader(const Backend *currentBackend);

  private:
    /**
     * \brief Attempt next connection from the list managed by the specified
     * `ConnectionManager` object.
     * \param connectionManager shared pointer to `ConnectionManager`
     */
    void attemptConnection(
        const std::shared_ptr<ConnectionManager> &connectionManager);

    /**
     * \brief Second stage of attempting a connection with a list of resolved
     * endpoints.
     * \param connectionManager shared pointer to `ConnectionManager`
     */
    void attemptResolvedConnection(
        const std::shared_ptr<ConnectionManager> &connectionManager);

    /**
     * \brief Third stage of attempting a connection with a fully resolved
     * IP+port
     * \param endpoint peer endpoint to be connected with
     */
    void attemptEndpointConnection(
        boost::asio::ip::tcp::endpoint            endpoint,
        const std::shared_ptr<ConnectionManager> &connectionManager);

    /**
     * \brief Start establishing a connection for this incoming session
     */
    void establishConnection();

    /**
     * \brief Send data stored in connector's buffer and originated on the
     * proxy side
     */
    void sendSyntheticData();

    /**
     * \brief Handle the incoming data that's been received
     * \param direction specifies direction of the data flow (ingress/egress)
     */
    void handleData(FlowType direction);

    /**
     * \brief Read more data into the session
     * \param direction specifies direction of the data flow (ingress/egress)
     */
    void readData(FlowType direction);

    /**
     * \brief Put the supplied data onto the outgoing socket, then re-read
     * \param direction specifies direction of the data flow (ingress/egress)
     * \param writeSocket to write the data
     * \param data to be written onto the outgoing socket
     */
    void handleWriteData(FlowType                  direction,
                         MaybeSecureSocketAdaptor &writeSocket,
                         Buffer                    data);

    /**
     * \brief Handle errors on an established connection
     * \param action specifies action information
     * \param direction specifies direction of the data flow (ingress/egress)
     * \param ec specifies error code
     */
    void handleSessionError(const char *              action,
                            FlowType                  direction,
                            boost::system::error_code ec);

    /**
     * \brief Handle errors while establishing a connection
     * \param action specifies action information
     * \param ec specifies error code
     * \param connectionManager shared pointer to `ConnectionManager`
     */
    void handleConnectionError(
        const char *                              action,
        boost::system::error_code                 ec,
        const std::shared_ptr<ConnectionManager> &connectionManager);

    /**
     * \brief Disconnect both sockets forcibly
     */
    void performDisconnectBoth();

    /**
     * \brief Copy any remaining data from the incoming buffer to a new buffer
     * for the next read operation to append to.
     * \param direction specifies direction of the data flow (ingress/egress)
     * \param remaining specifies remaining buffer data
     */
    inline void copyRemaining(FlowType direction, const Buffer &remaining);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the socket to read from
     */
    inline MaybeSecureSocketAdaptor &readSocket(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a buffer to used for reading into
     */
    inline Buffer readBuffer(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the amount of data already in the read
     * buffer
     */
    inline std::size_t &waterMark(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the time we last saw traffic
     */
    inline TimePoint &timePoint(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the ownership handle of the primary
     * buffer
     */
    inline BufferHandle &bufferHandle(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the ownership handle of the secondary
     * buffer
     */
    inline BufferHandle &bufferWriteDataHandle(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to the time when the last read started
     */
    inline Session::TimePoint &startedAt(FlowType direction);

    /**
     * \param direction specifies direction of the data flow (ingress/egress)
     * \return a mutable reference to whether we are currently reading a
     * message off the wire or not.
     */
    inline bool &currentlyReading(FlowType direction);

    /**
     * \return true if communication with server socket is secured, otherwise
     * false. Represents whether clients communicate with proxy using TLS.
     */
    inline bool isSecureServerSocket();
};

inline MaybeSecureSocketAdaptor &Session::readSocket(FlowType direction)
{
    return (direction == FlowType::INGRESS) ? d_serverSocket : d_clientSocket;
}

inline void Session::copyRemaining(FlowType direction, const Buffer &remaining)
{
    if (direction == FlowType::INGRESS) {
        if (remaining.size()) {
            d_serverWaterMark = remaining.size();
            d_serverWriteDataHandle.swap(d_serverDataHandle);
            d_bufferPool_p->acquireBuffer(&d_serverDataHandle,
                                          Frame::getMaxFrameSize());
            memcpy(
                d_serverDataHandle.data(), remaining.ptr(), d_serverWaterMark);
        }
        else {
            d_serverWaterMark = 0;
        }
    }
    else {
        if (remaining.size()) {
            d_clientWaterMark = remaining.size();
            d_clientWriteDataHandle.swap(d_clientDataHandle);
            d_bufferPool_p->acquireBuffer(&d_clientDataHandle,
                                          Frame::getMaxFrameSize());
            memcpy(
                d_clientDataHandle.data(), remaining.ptr(), d_clientWaterMark);
        }
        else {
            d_clientWaterMark = 0;
        }
    }
}

inline Buffer Session::readBuffer(FlowType direction)
{
    if (direction == FlowType::INGRESS) {
        Buffer readBuf(d_serverDataHandle.data(), d_serverDataHandle.size());
        readBuf.seek(d_serverWaterMark);
        return readBuf;
    }
    else {
        Buffer readBuf(d_clientDataHandle.data(), d_clientDataHandle.size());
        readBuf.seek(d_clientWaterMark);
        return readBuf;
    }
}

inline SessionState &Session::state()
{
    return d_sessionState;
}

inline const SessionState &Session::state() const
{
    return d_sessionState;
}

inline std::size_t &Session::waterMark(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_serverWaterMark
                                          : d_clientWaterMark;
}

inline Session::TimePoint &Session::timePoint(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_ingressWaitingSince
                                          : d_egressWaitingSince;
}

inline BufferHandle &Session::bufferHandle(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_serverDataHandle
                                          : d_clientDataHandle;
}

inline BufferHandle &Session::bufferWriteDataHandle(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_serverWriteDataHandle
                                          : d_clientWriteDataHandle;
}

inline Session::TimePoint &Session::startedAt(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_ingressStartedAt
                                          : d_egressStartedAt;
}

inline bool &Session::currentlyReading(FlowType direction)
{
    return direction == FlowType::INGRESS ? d_ingressCurrentlyReading
                                          : d_egressCurrentlyReading;
}

inline bool Session::isSecureServerSocket()
{
    return d_serverSocket.isSecure();
}

}
}

#endif
