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

#include <amqpprox_socketintercepttestadaptor.h>

#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace Bloomberg {
namespace amqpprox {

SocketInterceptTestAdaptor::SocketInterceptTestAdaptor(TestSocketState &state)
: d_state(state)
{
    d_state.handleTransition(
        [this](TestSocketState::Item *item) { this->drive(item); });
}

void SocketInterceptTestAdaptor::setSecure(bool secure)
{
    d_state.recordCall("setSecure");
    d_state.currentState().d_secure = secure;
}

bool SocketInterceptTestAdaptor::isSecure() const
{
    d_state.recordCall("isSecure");
    return d_state.currentState().d_secure;
}

void SocketInterceptTestAdaptor::refreshSocketContext()
{
    d_state.recordCall("refreshSocketContext");
}

SocketInterceptTestAdaptor::endpoint
SocketInterceptTestAdaptor::remote_endpoint(boost::system::error_code &ec)
{
    d_state.recordCallCheck("remote_endpoint", ec);
    return d_state.currentState().d_remote;
}

SocketInterceptTestAdaptor::endpoint
SocketInterceptTestAdaptor::local_endpoint(boost::system::error_code &ec)
{
    d_state.recordCallCheck("local_endpoint", ec);
    return d_state.currentState().d_local;
}

void SocketInterceptTestAdaptor::shutdown(boost::system::error_code &ec)
{
    d_state.recordCallCheck("shutdown", ec);
}

void SocketInterceptTestAdaptor::close(boost::system::error_code &ec)
{
    d_state.recordCallCheck("close", ec);
}

void SocketInterceptTestAdaptor::setDefaultOptions(
    boost::system::error_code &ec)
{
    d_state.recordCallCheck("setDefaultOptions", ec);
}

std::size_t
SocketInterceptTestAdaptor::available(boost::system::error_code &ec)
{
    d_state.recordCallCheck("available", ec);

    if (d_currentData) {
        return d_currentData->d_value.size();
    }

    throw std::runtime_error("No item available");
}

void SocketInterceptTestAdaptor::async_connect(const endpoint &peer_endpoint,
                                               AsyncConnectHandler handler)
{
    d_state.recordCall("async_connect");
    d_connectionHandler = handler;
}

void SocketInterceptTestAdaptor::async_handshake(handshake_type        type,
                                                 AsyncHandshakeHandler handler)
{
    d_state.recordCall("async_handshake");
    d_handshakeHandler = handler;
}

void SocketInterceptTestAdaptor::async_write_some(
    const std::vector<std::pair<const void *, size_t>> &buffers,
    AsyncWriteHandler                                   handler)
{
    d_state.recordCall("async_write_some");
    boost::system::error_code ec;
    std::size_t               sz = 0;
    for (auto &buf : buffers) {
        sz += d_state.recordData(buf.first, buf.second, ec);
    }
    handler(ec, sz);
}

std::size_t SocketInterceptTestAdaptor::read_some(
    const std::vector<std::pair<void *, size_t>> &buffers,
    boost::system::error_code &                   ec)
{
    d_state.recordCall("read_some");

    if (!d_currentData) {
        throw std::runtime_error("No data for read_some when expected");
    }

    std::size_t sz  = 0;
    auto &      src = d_currentData->d_value;

    for (auto &buf : buffers) {
        auto copySize = std::min(src.size(), buf.second);
        memcpy(buf.first, src.data(), copySize);
        src.erase(src.begin(), src.begin() + copySize);
        sz += copySize;

        if (src.empty()) {
            break;
        }
    }

    if (src.empty()) {
        d_currentData = nullptr;
    }

    return sz;
}

void SocketInterceptTestAdaptor::async_read_some(
    const std::vector<std::pair<void *, size_t>> &buffers,
    AsyncReadHandler                              handler)
{
    d_state.recordCall("async_read_some");
    d_readHandler = handler;
}

void SocketInterceptTestAdaptor::drive(TestSocketState::Item *item)
{
    if (auto hshake = std::get_if<TestSocketState::HandshakeComplete>(item)) {
        auto handler       = d_handshakeHandler;
        d_handshakeHandler = AsyncHandshakeHandler();
        if (handler) {
            handler(hshake->d_ec);
        }
        else {
            throw std::runtime_error(
                "No handshake handler, handshake not expected");
        }
    }
    else if (auto connect =
                 std::get_if<TestSocketState::ConnectComplete>(item)) {
        auto handler        = d_connectionHandler;
        d_connectionHandler = AsyncConnectHandler();
        if (handler) {
            handler(connect->d_ec);
        }
        else {
            throw std::runtime_error(
                "No connection handler, connection not expected");
        }
    }
    else if (auto data = std::get_if<TestSocketState::Data>(item)) {
        if (d_readHandler) {
            auto handler  = d_readHandler;
            d_readHandler = AsyncReadHandler();
            d_currentData = data;
            handler(data->d_ec, data->d_value.size());
        }
    }
    else {
        throw std::runtime_error("No read handler, data not expected");
    }
}

}
}
