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

#ifndef BLOOMBERG_AMQPPROX_SOCKETINTERCEPTTESTADAPTOR
#define BLOOMBERG_AMQPPROX_SOCKETINTERCEPTTESTADAPTOR

#include <amqpprox_socketinterceptinterface.h>

#include <amqpprox_testsocketstate.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Adapt between SocketInterceptInterface and TestSocketState
 *
 * This component provides the glue to be able to provide the behaviour for the
 * SocketInterceptInterface from a TestSocketState component. This allows it to
 * be injected into the SocketIntercept and test the important components such
 * as Session.
 *
 * The interface here is primarily that of SocketInterceptInterface so the
 * documentation for that will not be duplicated here.
 */
class SocketInterceptTestAdaptor : public SocketInterceptInterface {
    TestSocketState       &d_state;
    AsyncReadHandler       d_readHandler;
    AsyncConnectHandler    d_connectionHandler;
    AsyncHandshakeHandler  d_handshakeHandler;
    TestSocketState::Data *d_currentData;

    /**
     * \brief Private function that receives asynchronous events
     *
     * This private member function is registered into the TestSocketState to
     * provide the event triggers that provide the notifications of new data to
     * the asynchronous handlers that are registered in this class for
     * async_read in particular, but also the specialised ones for handshaking
     * and connection. This effectively forms part of a visitor pattern out of
     * the TestSocketState's state during test driving and adapts these
     * visitations into async callbacks that the operating system would
     * normally provide the socket.
     *
     * \param item The item that's being visited from the TestSocketState
     */
    void drive(TestSocketState::Item *item);

  public:
    // PUBLIC TYPES
    using endpoint       = boost::asio::ip::tcp::endpoint;
    using handshake_type = boost::asio::ssl::stream_base::handshake_type;
    using AsyncReadHandler =
        std::function<void(boost::system::error_code, std::size_t)>;
    using AsyncHandshakeHandler =
        std::function<void(boost::system::error_code)>;
    using AsyncConnectHandler = std::function<void(boost::system::error_code)>;
    using AsyncShutdownHandler =
        std::function<void(boost::system::error_code)>;

    // CREATORS
    /**
     * \brief Construct an adaptor from the given test state
     * \param state The test state to use for this component
     */
    explicit SocketInterceptTestAdaptor(TestSocketState &state);

    // SocketInterceptInterface Implementations
    virtual void setSecure(bool secure) override;
    virtual void refreshSocketContext() override;

    virtual endpoint remote_endpoint(boost::system::error_code &ec) override;

    virtual endpoint local_endpoint(boost::system::error_code &ec) override;

    virtual void close(boost::system::error_code &ec) override;

    virtual void setDefaultOptions(boost::system::error_code &ec) override;

    virtual std::size_t available(boost::system::error_code &ec) override;

    virtual void async_shutdown(AsyncShutdownHandler handler) override;

    virtual void async_connect(const endpoint     &peer_endpoint,
                               AsyncConnectHandler handler) override;

    virtual void async_handshake(handshake_type        type,
                                 AsyncHandshakeHandler handler) override;

    virtual void async_write_some(
        const std::vector<std::pair<const void *, size_t>> &buffers,
        AsyncWriteHandler                                   handler) override;

    virtual std::size_t
    read_some(const std::vector<std::pair<void *, size_t>> &buffers,
              boost::system::error_code                    &ec) override;

    virtual void
    async_read_some(const std::vector<std::pair<void *, size_t>> &buffers,
                    AsyncReadHandler handler) override;
};

}
}

#endif
