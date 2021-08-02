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

#include <amqpprox_logging.h>
#include <amqpprox_socketintercept.h>

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

    boost::asio::io_service &                              d_ioService;
    std::optional<std::reference_wrapper<SocketIntercept>> d_intercept;
    std::unique_ptr<stream_type> d_socket;
    bool                         d_secured;
    bool                         d_handshook;

  public:
    typedef typename stream_type::executor_type executor_type;

#ifdef SOCKET_TESTING
    MaybeSecureSocketAdaptor(boost::asio::io_service &ioService,
                             SocketIntercept &        intercept,
                             bool                     secured)
    : d_ioService(ioService)
    , d_intercept(intercept)
    , d_socket()
    , d_secured(secured)
    , d_handshook(false)
    {
    }
#endif

    MaybeSecureSocketAdaptor(boost::asio::io_service &  ioService,
                             boost::asio::ssl::context &context,
                             bool                       secured)
    : d_ioService(ioService)
    , d_intercept()
    , d_socket(std::make_unique<stream_type>(ioService, context))
    , d_secured(secured)
    , d_handshook(false)
    {
    }

    MaybeSecureSocketAdaptor(MaybeSecureSocketAdaptor &&src)
    : d_ioService(src.d_ioService)
    , d_intercept(src.d_intercept)
    , d_socket(std::move(src.d_socket))
    , d_secured(src.d_secured)
    , d_handshook(src.d_handshook)
    {
        src.d_socket = std::unique_ptr<stream_type>();
        src.d_secured   = false;
        src.d_handshook = false;
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

    void shutdown(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().shutdown(ec);
            return;
        }

        if (isSecure()) {
            d_socket->shutdown(ec);

            // If the TLS shutdown fails we still want to shutdown and close
            // the socket so we fall through
        }

        d_socket->next_layer().shutdown(
            boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    void close(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            d_intercept.value().get().close(ec);
            return;
        }

        d_socket->next_layer().close(ec);
    }

    std::size_t available(boost::system::error_code &ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().available(ec);
        }

        return d_socket->next_layer().available(ec);
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
                          boost::system::error_code &  ec)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().read_some(buffers, ec);
        }

        if (isSecure()) {
            return d_socket->read_some(buffers, ec);
        }
        else {
            return d_socket->next_layer().read_some(buffers, ec);
        }
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                  void(boost::system::error_code, std::size_t))
    async_read_some(const MutableBufferSequence &buffers,
                    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
    {
        if (BOOST_UNLIKELY(d_intercept.has_value())) {
            return d_intercept.value().get().async_read_some(buffers, handler);
        }

        if (isSecure()) {
            return d_socket->async_read_some(buffers, handler);
        }
        else {
            return d_socket->next_layer().async_read_some(buffers, handler);
        }
    }
};

}
}

#endif
