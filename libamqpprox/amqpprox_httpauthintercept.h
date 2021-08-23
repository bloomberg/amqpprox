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
#ifndef BLOOMBERG_AMQPPROX_HTTPAUTHINTERCEPT
#define BLOOMBERG_AMQPPROX_HTTPAUTHINTERCEPT

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_dnsresolver.h>
#include <authrequest.pb.h>

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
using tcp       = boost::asio::ip::tcp;
}

class HttpAuthIntercept
: public AuthInterceptInterface,
  public std::enable_shared_from_this<HttpAuthIntercept> {
    boost::asio::io_service &d_ioService;
    std::string              d_hostname;
    std::string              d_port;
    std::string              d_target;
    DNSResolver *            d_dnsResolver_p;
    mutable std::mutex       d_mutex;

    void
    onResolve(std::shared_ptr<beast::http::request<beast::http::string_body>>
                                               request,
              const ReceiveResponseCb &        responseCb,
              const boost::system::error_code &ec,
              std::vector<tcp::endpoint>       results);
    void
    onConnect(std::shared_ptr<beast::tcp_stream> stream,
              std::shared_ptr<beast::http::request<beast::http::string_body>>
                                       request,
              const ReceiveResponseCb &responseCb,
              beast::error_code        ec,
              tcp::resolver::results_type::endpoint_type);
    void
    onWrite(std::shared_ptr<beast::tcp_stream> stream,
            std::shared_ptr<beast::http::request<beast::http::string_body>>
                                     request,
            const ReceiveResponseCb &responseCb,
            beast::error_code        ec,
            std::size_t              bytes_transferred);
    void
    onRead(std::shared_ptr<beast::flat_buffer> buffer,
           std::shared_ptr<beast::tcp_stream>  stream,
           std::shared_ptr<beast::http::response<beast::http::string_body>>
                                    response,
           const ReceiveResponseCb &responseCb,
           beast::error_code        ec,
           std::size_t              bytes_transferred);

  public:
    // CREATORS
    HttpAuthIntercept(boost::asio::io_service &ioService,
                      const std::string &      hostname,
                      const std::string &      port,
                      const std::string &      target,
                      DNSResolver *            dnsResolver);

    virtual ~HttpAuthIntercept() override = default;

    // MANIPULATORS
    /**
     * \brief It gets all the information required to authenticate from client
     * in requestBody parameter and invoke callback function to provide
     * response.
     * \param requestBody request data payload
     * \param responseCb Callbak function with response values
     */
    virtual void authenticate(const authproto::AuthRequest authRequestData,
                              const ReceiveResponseCb &responseCb) override;

    // ACCESSORS
    /**
     * \brief Print information about route auth gate service
     * \param os output stream object
     */
    virtual void print(std::ostream &os) const override;
};

}
}

#endif
