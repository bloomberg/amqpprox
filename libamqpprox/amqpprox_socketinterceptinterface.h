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
#ifndef BLOOMBERG_AMQPPROX_SOCKETINTERCEPTINTERFACE
#define BLOOMBERG_AMQPPROX_SOCKETINTERCEPTINTERFACE

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/system/error_code.hpp>

namespace Bloomberg {
namespace amqpprox {

/** \brief Provide a virtual interface for socket operations
 *
 * This is a pure virtual interface derived from the non-virtual interfaces
 * expected by boost ASIO. This is predominantly designed to benefit testing
 * where we want to be able to inject differing behaviours in tests.
 */
class SocketInterceptInterface {
  public:
    using endpoint       = boost::asio::ip::tcp::endpoint;
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;
    using AsyncReadHandler =
        std::function<void(boost::system::error_code, std::size_t)>;
    using AsyncWriteHandler =
        std::function<void(boost::system::error_code, std::size_t)>;
    using AsyncHandshakeHandler =
        std::function<void(boost::system::error_code)>;
    using AsyncConnectHandler = std::function<void(boost::system::error_code)>;

    /**
     * \brief Set the socket into secure (using TLS) or unsecure (plaintext)
     *
     * Note that this will normally need to be called prior to any handshake,
     * occuring, and shouldn't be changed afterwards. This is only manipulator
     * because the socket initially has to start in non-secure mode and may
     * need to do a proxy-protocol interchange before handshaking.
     *
     * \param secure True for using TLS
     */
    virtual void setSecure(bool secure) = 0;

    /**
     * \brief Return if the socket is in secure mode
     */
    virtual bool isSecure() const       = 0;

    /**
     * \brief Reset the socket's context as if it is a fresh socket
     *
     * This is used to reset the context before it can be used to be accepted
     * into, this allows any changes to the TLS context to be picked up for new
     * connections being accepted.
     */
    virtual void refreshSocketContext() = 0;

    /**
     * \brief Return the remote (peer) TCP endpoint
     * \param ec The error code set by the operation, only changed on error.
     * \return The TCP endpoint
     */
    virtual endpoint remote_endpoint(boost::system::error_code &ec) = 0;

    /**
     * \brief Return the local TCP endpoint
     * \param ec The error code set by the operation, only changed on error.
     * \return The TCP endpoint
     */
    virtual endpoint local_endpoint(boost::system::error_code &ec) = 0;

    /**
     * \brief Shutdown the socket for read and write.
     * \param ec The error code set by the operation, only changed on error.
     */
    virtual void shutdown(boost::system::error_code &ec) = 0;

    /**
     * \brief Close the socket
     * \param ec The error code set by the operation, only changed on error.
     */
    virtual void close(boost::system::error_code &ec) = 0;

    /**
     * \brief Set the default options on the socket.
     *
     * This will typically be items such as TCP_NODELAY and disabling
     * lingering.
     *
     * \param ec The error code set by the operation, only changed on error.
     */
    virtual void setDefaultOptions(boost::system::error_code &ec) = 0;

    /**
     * \brief Return how much data is available to read without blocking
     * \param ec The error code set by the operation, only changed on error.
     * \return The number of bytes available to read
     */
    virtual std::size_t available(boost::system::error_code &ec) = 0;

    /**
     * \brief Initiate a connection with the socket
     * \param peer_endpoint The endpoint to connect to
     * \param handler The completion handler to invoke on success/error
     */
    virtual void async_connect(const endpoint &    peer_endpoint,
                               AsyncConnectHandler handler) = 0;

    /**
     * \brief Initiate handshaking for TLS
     *
     * NB: This will generally always be called but expect to be a no-op on
     * non-secure connections.
     *
     * \param handshake_type TLS handshake type (from ASIO)
     * \param handler The completion handler to invoke on success/error
     */
    virtual void async_handshake(handshake_type        type,
                                 AsyncHandshakeHandler handler) = 0;

    /**
     * \brief Write some data onto the socket
     *
     * This method will write some data onto the socket interface as a sequence
     * of const buffers. The return value from it can be less than the total
     * size of the buffers if a partial write occurs.
     *
     * \param buffers The buffer sequence to be written.
     * \param handler The completion handler to invoke on success/error, it
     *                will also return the amount of bytes written.
     */
    virtual void
    async_write_some(const std::vector<std::pair<const void *, size_t>> &buffers,
                     AsyncWriteHandler handler) = 0;

    /**
     * \brief Synchronously read some data onto the socket
     *
     * This method will read some data from the socket interface into a
     * sequence of buffers. The return value from it can be less than the total
     * size of the buffers if a partial read occurs.
     *
     * \param buffers The buffer sequence to be written into.
     * \param ec The error code set by the operation, only changed on error.
     * \return the number of bytes read by the operation
     */
    virtual std::size_t
    read_some(const std::vector<std::pair<void *, size_t>> &buffers,
              boost::system::error_code &                   ec) = 0;

    /**
     * \brief Asynchronously read some data onto the socket
     *
     * This method will read some data from the socket interface into a
     * sequence of buffers. The return value from it can be less than the total
     * size of the buffers if a partial read occurs.
     *
     * \param buffers The buffer sequence to be written into.
     * \param handler The completion handler to invoke on success/error, it
     *                will also return the amount of bytes read.
     */
    virtual void
    async_read_some(const std::vector<std::pair<void *, size_t>> &buffers,
                    AsyncReadHandler                              handler) = 0;
};

}
}

#endif
