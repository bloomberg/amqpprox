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
#ifndef BLOOMBERG_AMQPPROX_AUTHINTERCEPTINTERFACE
#define BLOOMBERG_AMQPPROX_AUTHINTERCEPTINTERFACE

#include <functional>
#include <iostream>

#include <boost/asio.hpp>

namespace Bloomberg {
namespace amqpprox {

namespace authproto {
class AuthRequest;
class AuthResponse;
}

/**
 * \brief Provide a pure virtual interface for authn/authz operations
 */
class AuthInterceptInterface {
  protected:
    boost::asio::io_context &d_ioContext;

  public:
    /**
     * \brief Callback function to return response allow/deny with reason after
     * authenticating client connection.
     */
    typedef std::function<void(const authproto::AuthResponse &)>
        ReceiveResponseCb;

    // CREATORS
    explicit AuthInterceptInterface(boost::asio::io_context &ioContext);

    virtual ~AuthInterceptInterface() = default;

    // MANIPULATORS
    /**
     * \brief It gets all the information required to authenticate from client
     * in authRequestData parameter and invoke callback function to provide
     * response.
     * \param authRequestData auth request data payload
     * \param responseCb Callbak function with response values
     */
    virtual void authenticate(const authproto::AuthRequest authRequestData,
                              const ReceiveResponseCb     &responseCb) = 0;

    // ACCESSORS
    /**
     * \brief Print information about route auth gate service
     * \param os output stream object
     */
    virtual void print(std::ostream &os) const = 0;
};

}
}

#endif
