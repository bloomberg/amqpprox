/*
** Copyright 2021 Bloomberg Finance L.P.
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

#include <amqpprox_httpauthintercept.h>

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_logging.h>
#include <authrequest.pb.h>
#include <authresponse.pb.h>

#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace Bloomberg {
namespace amqpprox {

namespace {
namespace beast = boost::beast;

const int TIMEOUT_SECONDS = 30;
int       HTTP_VERSION    = 11;  // HTTP/1.1 version
}

HttpAuthIntercept::HttpAuthIntercept(boost::asio::io_context &ioContext,
                                     const std::string       &hostname,
                                     const std::string       &port,
                                     const std::string       &target,
                                     DNSResolver             *dnsResolver)
: AuthInterceptInterface(ioContext)
, d_ioContext(ioContext)
, d_hostname(hostname)
, d_port(port)
, d_target(target)
, d_dnsResolver_p(dnsResolver)
, d_mutex()
{
}

void HttpAuthIntercept::authenticate(
    const authproto::AuthRequest authRequestData,
    const ReceiveResponseCb     &responseCb)
{
    std::shared_ptr<beast::http::request<beast::http::string_body>> request =
        std::make_shared<beast::http::request<beast::http::string_body>>();
    request->version(HTTP_VERSION);
    request->method(beast::http::verb::post);
    request->target(d_target);
    request->set(beast::http::field::host, d_hostname + ":" + d_port);
    request->set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request->set(beast::http::field::content_type, "application/octet-stream");
    std::string serializedRequestBody;
    if (!authRequestData.SerializeToString(&serializedRequestBody)) {
        const std::string errorMsg =
            "Unable to serialize auth request data for http service.";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }
    request->body() = serializedRequestBody;
    request->prepare_payload();

    // Look up the domain name
    d_dnsResolver_p->resolve(d_hostname,
                             d_port,
                             std::bind(&HttpAuthIntercept::onResolve,
                                       shared_from_this(),
                                       request,
                                       responseCb,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
}

void HttpAuthIntercept::onResolve(
    std::shared_ptr<beast::http::request<beast::http::string_body>> request,
    const ReceiveResponseCb                                        &responseCb,
    const boost::system::error_code                                &ec,
    std::vector<tcp::endpoint>                                      results)
{
    if (ec) {
        const std::string errorMsg =
            "Unable to resolve hostname " + d_hostname +
            ", Error: {category: " + ec.category().name() +
            ", message: " + ec.message() +
            ", value: " + std::to_string(ec.value()) + "}";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }

    std::shared_ptr<beast::tcp_stream> stream =
        std::make_shared<beast::tcp_stream>(
            boost::asio::make_strand(d_ioContext));
    // Set a timeout on the operation
    stream->expires_after(std::chrono::seconds(TIMEOUT_SECONDS));

    // Make the connection on the IP address we get from a lookup
    stream->async_connect(
        results,
        beast::bind_front_handler(&HttpAuthIntercept::onConnect,
                                  shared_from_this(),
                                  stream,
                                  request,
                                  responseCb));
}

void HttpAuthIntercept::onConnect(
    std::shared_ptr<beast::tcp_stream>                              stream,
    std::shared_ptr<beast::http::request<beast::http::string_body>> request,
    const ReceiveResponseCb                                        &responseCb,
    beast::error_code                                               ec,
    tcp::resolver::results_type::endpoint_type)
{
    if (ec) {
        const std::string errorMsg =
            "Unable to connect hostname " + d_hostname + " on port " + d_port +
            ", Error: {category: " + ec.category().name() +
            ", message: " + ec.message() +
            ", value: " + std::to_string(ec.value()) + "}";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }

    // Set a timeout on the operation
    stream->expires_after(std::chrono::seconds(TIMEOUT_SECONDS));

    // Send the HTTP request to the remote host
    beast::http::async_write(
        *stream,
        *request,
        beast::bind_front_handler(&HttpAuthIntercept::onWrite,
                                  shared_from_this(),
                                  stream,
                                  request,
                                  responseCb));
}

void HttpAuthIntercept::onWrite(
    std::shared_ptr<beast::tcp_stream> stream,
    std::shared_ptr<beast::http::request<
        beast::http::string_body>>,  // Extend the lifetime of request object
    const ReceiveResponseCb &responseCb,
    beast::error_code        ec,
    std::size_t              bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        const std::string errorMsg =
            "Unable to send http request to hostname " + d_hostname +
            " on port " + d_port +
            ", Error: {category: " + ec.category().name() +
            ", message: " + ec.message() +
            ", value: " + std::to_string(ec.value()) + "}";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }

    std::shared_ptr<beast::http::response<beast::http::string_body>> response =
        std::make_shared<beast::http::response<beast::http::string_body>>();

    std::shared_ptr<beast::flat_buffer> buffer =
        std::make_shared<beast::flat_buffer>();

    // Receive the HTTP response
    beast::http::async_read(
        *stream,
        *buffer,
        *response,
        beast::bind_front_handler(&HttpAuthIntercept::onRead,
                                  shared_from_this(),
                                  buffer,
                                  stream,
                                  response,
                                  responseCb));
}

void HttpAuthIntercept::onRead(
    std::shared_ptr<beast::flat_buffer>,  // Buffer must persist between reads
    std::shared_ptr<beast::tcp_stream>                               stream,
    std::shared_ptr<beast::http::response<beast::http::string_body>> response,
    const ReceiveResponseCb &responseCb,
    beast::error_code        ec,
    std::size_t              bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        const std::string errorMsg =
            "Unable to receive http response from hostname " + d_hostname +
            " on port " + d_port +
            ", Error: {category: " + ec.category().name() +
            ", message: " + ec.message() +
            ", value: " + std::to_string(ec.value()) + "}";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }

    authproto::AuthResponse authResponseData;
    if (!authResponseData.ParseFromString(response->body())) {
        const std::string errorMsg = "Unable to deserialize auth response "
                                     "data received from http service.";
        LOG_ERROR << errorMsg;
        authproto::AuthResponse errorResponseData;
        errorResponseData.set_result(authproto::AuthResponse::DENY);
        errorResponseData.set_reason(errorMsg);
        responseCb(errorResponseData);
        return;
    }
    LOG_TRACE << "Response from auth route gate service at " << d_hostname
              << ":" << d_port << d_target << ": "
              << "[ Auth Result: "
              << ((authResponseData.result() == authproto::AuthResponse::ALLOW)
                      ? "ALLOW"
                      : "DENY")
              << ", Reason: " << authResponseData.reason()
              << ((authResponseData.has_authdata())
                      ? (", Auth mechanism: " +
                         authResponseData.authdata().authmechanism())
                      : "")
              << " ]";
    responseCb(authResponseData);

    // Gracefully close the socket
    stream->socket().shutdown(tcp::socket::shutdown_both, ec);

    // TODO shouldn't we also call stream->socket().close()?
    // not_connected happens sometimes so don't bother reporting it.
    if (ec && ec != beast::errc::not_connected) {
        const std::string errorMsg =
            "Unable to close socket and shutdown connection gracefully for "
            "auth route gate service at " +
            d_hostname + ":" + d_port + d_target +
            ", Error: {category: " + ec.category().name() +
            ", message: " + ec.message() +
            ", value: " + std::to_string(ec.value()) + "}";
        LOG_WARN << errorMsg;
        return;
    }

    // If we get here then the connection is closed gracefully
}

void HttpAuthIntercept::print(std::ostream &os) const
{
    std::lock_guard<std::mutex> lg(d_mutex);
    os << "HTTP Auth service will be used to authn/authz client connections: "
          "http://"
       << d_hostname << ":" << d_port << d_target << "\n";
}
}
}
