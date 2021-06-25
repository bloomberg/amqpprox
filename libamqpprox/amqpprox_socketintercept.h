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
#ifndef BLOOMBERG_AMQPPROX_SOCKETINTERCEPT
#define BLOOMBERG_AMQPPROX_SOCKETINTERCEPT

#include <amqpprox_socketinterceptinterface.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <stdexcept>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Override socket operations
 *
 * This component provides an adapter between the non-virtual interface and
 * templated interface that boost asio socket/stream implementation require and
 * a virtual interface, designed predominantly for testing. The
 * MaybeSecureSocketAdaptor delegates to this class to separate the production
 * runtime versus testing concerns.
 */
class SocketIntercept {
    using AsyncReadHandler =
        std::function<void(boost::system::error_code, std::size_t)>;
    using AsyncHandshakeHandler =
        std::function<void(boost::system::error_code)>;
    using AsyncConnectHandler = std::function<void(boost::system::error_code)>;

    SocketInterceptInterface &d_impl;

  public:
    using endpoint       = boost::asio::ip::tcp::endpoint;
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;

    /**
     * \brief Construct the non-virtual interface with the virtual interface
     * \param impl Virtual interface for the implementation of the intercept
     */
    explicit SocketIntercept(SocketInterceptInterface &impl)
    : d_impl(impl)
    {
    }

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
    void setSecure(bool secure);

    /**
     * \brief Return if the socket is in secure mode
     */
    bool isSecure() const;

    /**
     * \brief Reset the socket's context as if it is a fresh socket
     *
     * This is used to reset the context before it can be used to be accepted
     * into, this allows any changes to the TLS context to be picked up for new
     * connections being accepted.
     */
    void refreshSocketContext();

    /**
     * \brief Return the remote (peer) TCP endpoint
     * \param ec The error code set by the operation, only changed on error.
     * \return The TCP endpoint
     */
    endpoint remote_endpoint(boost::system::error_code &ec);

    /**
     * \brief Return the local TCP endpoint
     * \param ec The error code set by the operation, only changed on error.
     * \return The TCP endpoint
     */
    endpoint local_endpoint(boost::system::error_code &ec);

    /**
     * \brief Shutdown the socket for read and write.
     * \param ec The error code set by the operation, only changed on error.
     */
    void shutdown(boost::system::error_code &ec);

    /**
     * \brief Close the socket
     * \param ec The error code set by the operation, only changed on error.
     */
    void close(boost::system::error_code &ec);

    /**
     * \brief Set the default options on the socket.
     *
     * This will typically be items such as TCP_NODELAY and disabling
     * lingering.
     *
     * \param ec The error code set by the operation, only changed on error.
     */
    void setDefaultOptions(boost::system::error_code &ec);

    /**
     * \brief Return how much data is available to read without blocking
     * \param ec The error code set by the operation, only changed on error.
     * \return The number of bytes available to read
     */
    std::size_t available(boost::system::error_code &ec);

    /**
     * \brief Initiate a connection with the socket
     * \param peer_endpoint The endpoint to connect to
     * \param handler The completion handler to invoke on success/error
     */
    template <typename ConnectHandler>
    void async_connect(const endpoint &peer_endpoint, ConnectHandler &&handler)
    {
        d_impl.async_connect(peer_endpoint, handler);
    }

    /**
     * \brief Initiate handshaking for TLS
     *
     * NB: This will generally always be called but expect to be a no-op on
     * non-secure connections.
     *
     * \param handshake_type TLS handshake type (from ASIO)
     * \param handler The completion handler to invoke on success/error
     */
    template <typename HandshakeHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(HandshakeHandler,
                                  void(boost::system::error_code))
    async_handshake(handshake_type type,
                    BOOST_ASIO_MOVE_ARG(HandshakeHandler) handler)
    {
        d_impl.async_handshake(type, handler);
    }

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
    template <typename ConstBufferSequence, typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                  void(boost::system::error_code, std::size_t))
    async_write_some(const ConstBufferSequence &buffers,
                     BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
    {
        std::vector<std::pair<const void *, size_t>> bufs;
        auto begin = boost::asio::buffer_sequence_begin(buffers);
        auto end   = boost::asio::buffer_sequence_end(buffers);
        for (auto it = begin; it != end; ++it) {
            bufs.push_back(std::make_pair(it->data(), it->size()));
        }
        d_impl.async_write_some(bufs, handler);
    }

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
    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence &buffers,
                          boost::system::error_code &  ec)
    {
        std::vector<std::pair<void *, size_t>> bufs;
        auto begin = boost::asio::buffer_sequence_begin(buffers);
        auto end   = boost::asio::buffer_sequence_end(buffers);
        for (auto it = begin; it != end; ++it) {
            bufs.push_back(std::make_pair(it->data(), it->size()));
        }
        return d_impl.read_some(bufs, ec);
    }

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
    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                  void(boost::system::error_code, std::size_t))
    async_read_some(const MutableBufferSequence &buffers,
                    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
    {
        std::vector<std::pair<void *, size_t>> bufs;
        auto begin = boost::asio::buffer_sequence_begin(buffers);
        auto end   = boost::asio::buffer_sequence_end(buffers);
        for (auto it = begin; it != end; ++it) {
            bufs.push_back(std::make_pair(it->data(), it->size()));
        }
        d_impl.async_read_some(bufs, handler);
    }
};

}
}

#endif
