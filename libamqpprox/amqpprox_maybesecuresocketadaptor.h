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
#ifndef BLOOMBERG_AMQPPROX_MAYBESECURESOCKETADAPTOR
#define BLOOMBERG_AMQPPROX_MAYBESECURESOCKETADAPTOR

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <openssl/ssl.h>

#include <amqpprox_logging.h>
#include <amqpprox_socketintercept.h>

#include <functional>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief This class sits in the session class wrapping the ssl socket to
 * provide a unified interface to the read/write parts, which internally
 * switches on the d_secured to determine if to use the underlying socket when
 * not secured, or the top level functions which pass through openssl if it is
 * secured.
 */
class MaybeSecureSocketAdaptor {
    using stream_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    using endpoint    = boost::asio::ip::tcp::endpoint;
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;

    boost::asio::io_context                               &d_ioService;
    std::optional<std::reference_wrapper<SocketIntercept>> d_intercept;
    std::unique_ptr<stream_type>                           d_socket;
    bool                                                   d_secured;
    bool                                                   d_handshook;
    char                                                   d_smallBuffer;
    bool                                                   d_smallBufferSet;

  public:
    typedef typename stream_type::executor_type executor_type;

#ifdef SOCKET_TESTING
    MaybeSecureSocketAdaptor(boost::asio::io_context &ioService,
                             SocketIntercept         &intercept,
                             bool                     secured)
    : d_ioService(ioService)
    , d_intercept(intercept)
    , d_socket()
    , d_secured(secured)
    , d_handshook(false)
    , d_smallBuffer(0)
    , d_smallBufferSet(false)
    {
    }
#endif

    MaybeSecureSocketAdaptor(boost::asio::io_context   &ioService,
                             boost::asio::ssl::context &context,
                             bool                       secured)
    : d_ioService(ioService)
    , d_intercept()
    , d_socket(std::make_unique<stream_type>(ioService, context))
    , d_secured(secured)
    , d_handshook(false)
    , d_smallBuffer(0)
    , d_smallBufferSet(false)
    {
    }

    MaybeSecureSocketAdaptor(MaybeSecureSocketAdaptor &&src)
    : d_ioService(src.d_ioService)
    , d_intercept(src.d_intercept)
    , d_socket(std::move(src.d_socket))
    , d_secured(src.d_secured)
    , d_handshook(src.d_handshook)
    , d_smallBuffer(src.d_smallBuffer)
    , d_smallBufferSet(src.d_smallBufferSet)
    {
        src.d_socket         = std::unique_ptr<stream_type>();
        src.d_secured        = false;
        src.d_handshook      = false;
        src.d_smallBuffer    = 0;
        src.d_smallBufferSet = false;
    }

    boost::asio::ip::tcp::socket &socket() { return d_socket->next_layer(); }

    bool isSecure()
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().isSecure();
        }

        return d_secured && d_handshook;
    }

    void setSecure(bool secure)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().setSecure(secure);
            return;
        }

        d_secured = secure;
    }

    void setDefaultOptions(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().setDefaultOptions(ec);
            return;
        }

        auto &socket = d_socket->next_layer();

        socket.non_blocking(true, ec);
        if (ec) {
            LOG_TRACE << "Setting non_blocking on socket returned ec: " << ec;
            return;
        }

        boost::asio::ip::tcp::no_delay noDelayOption(true);
        socket.set_option(noDelayOption, ec);
        if (ec) {
            LOG_TRACE << "Setting nodelay on socket returned ec: " << ec;
            return;
        }

        boost::asio::ip::tcp::socket::keep_alive keepAlive(true);
        socket.set_option(keepAlive, ec);
        if (ec) {
            LOG_TRACE << "Setting keepalive on socket returned ec: " << ec;
            return;
        }
    }

    // Methods for compatibility with boost ssl stream / TCP stream

    endpoint remote_endpoint(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().remote_endpoint(ec);
        }

        return d_socket->next_layer().remote_endpoint(ec);
    }

    endpoint local_endpoint(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().local_endpoint(ec);
        }

        return d_socket->next_layer().local_endpoint(ec);
    }

    void close(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().close(ec);
            return;
        }

        d_socket->next_layer().close(ec);
    }

    /**
     * Indicates the number of bytes immediately readable out of the socket
     * For TLS connections this references the number of bytes which are
     * immediately available for reading from the current fully-read record
     */
    std::size_t available(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().available(ec);
        }

        if (d_secured) {
            return (d_smallBufferSet ? 1 : 0) +
                   SSL_pending(d_socket->native_handle());
        }
        else {
            return d_socket->next_layer().available(ec);
        }
    }

    template <typename ConnectHandler>
    void async_connect(const endpoint &peer_endpoint, ConnectHandler &&handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().async_connect(peer_endpoint, handler);
            return;
        }

        d_socket->next_layer().async_connect(peer_endpoint, handler);
    }

    template <typename HandshakeHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(HandshakeHandler,
                                  void(boost::system::error_code))
    async_handshake(handshake_type type,
                    BOOST_ASIO_MOVE_ARG(HandshakeHandler) handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().async_handshake(type, handler);
        }

        // Here we check the d_secured _only_ because this is prior to
        // handshaking and indicating the intent of the handshake to happen.
        // After the handshake all the socket operations should forward on to
        // the TLS socket wrapper not the underlying socket.
        if (d_secured) {
            d_handshook = true;
            return d_socket->async_handshake(type, handler);
        }
        else {
            boost::system::error_code ec;
            handler(ec);
        }
    }

    template <typename ShutdownHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ShutdownHandler,
                                  void(boost::system::error_code))
    async_shutdown(BOOST_ASIO_MOVE_ARG(ShutdownHandler) handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().async_shutdown(handler);
        }

        if (d_secured) {
            boost::system::error_code ec;
            d_socket->next_layer().shutdown(
                boost::asio::ip::tcp::socket::shutdown_receive, ec);
            if (ec) {
                LOG_DEBUG
                    << "Error shutting down receive direction for socket ec: "
                    << ec;
            }
            return d_socket->async_shutdown(handler);
        }
        else {
            // regular socket doesn't have async_shutdown, so just call the
            // handler directly
            boost::system::error_code ec;
            d_socket->next_layer().shutdown(
                boost::asio::ip::tcp::socket::shutdown_both, ec);
            handler(ec);
        }
    }

    template <typename ConstBufferSequence, typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                  void(boost::system::error_code, std::size_t))
    async_write_some(const ConstBufferSequence &buffers,
                     BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().async_write_some(buffers,
                                                              handler);
        }

        if (isSecure()) {
            return d_socket->async_write_some(buffers, handler);
        }
        else {
            return d_socket->next_layer().async_write_some(buffers, handler);
        }
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence &buffers,
                          boost::system::error_code   &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().read_some(buffers, ec);
        }

        if (isSecure()) {
            // Ensure we read the small-buffer-workaround if it has been used
            if (d_smallBufferSet && buffers.size() >= 1) {
                ((char *)buffers.data())[0] = d_smallBuffer;
                d_smallBufferSet            = false;

                MutableBufferSequence replacement(buffers);
                replacement += 1;

                size_t result = d_socket->read_some(replacement, ec);

                if (ec && result == 0) {
                    ec = boost::system::error_code();
                    // Pretend read_some succeeded this time around because
                    // there's one byte left over.
                    return 1;
                }

                return 1 + result;
            }

            return d_socket->read_some(buffers, ec);
        }
        else {
            return d_socket->next_layer().read_some(buffers, ec);
        }
    }

    /**
     * This async_read_some specialisation is required due to
     * https://github.com/chriskohlhoff/asio/issues/1015
     *
     * For TLS sockets we need to ensure we call this method with a buffer size
     * of at least one byte. This is handled by passing in a small buffer (1
     * byte). This byte is then passed back via `read_some`. The presense of
     * this byte is also represented in the return value of `available`.
     */
    template <typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                  void(boost::system::error_code, std::size_t))
    async_read_some(const boost::asio::null_buffers &null_buffer,
                    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().async_read_some(null_buffer,
                                                             handler);
        }

        if (isSecure()) {
            if (d_smallBufferSet) {
                // The reader missed a byte - invoke ssl
                // async_read_some(zero-sized-buffer) so the handler is
                // immediately invoked to collect this missing byte. This
                // codepath wasn't hit during testing, but it's left here for
                // completeness
                LOG_DEBUG << "Invoked async_read_some again before reading "
                             "data. Immediately invoking handler";

                return d_socket->async_read_some(
                    boost::asio::buffer(&d_smallBuffer, 0), handler);
            }

            // async_read_some with a one byte buffer to ensure we are only
            // called with useful progress
            return d_socket->async_read_some(
                boost::asio::buffer(&d_smallBuffer, sizeof(d_smallBuffer)),
                [this, handler](boost::system::error_code ec,
                                std::size_t               length) {
                    if (length != 0) {
                        d_smallBufferSet = true;
                    }

                    handler(ec, length);
                });
        }
        else {
            return d_socket->next_layer().async_read_some(null_buffer,
                                                          handler);
        }
    }
};
}
}

#endif
