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
#ifndef BLOOMBERG_AMQPPROX_DEFAULTAUTHINTERCEPT
#define BLOOMBERG_AMQPPROX_DEFAULTAUTHINTERCEPT

#include <amqpprox_authinterceptinterface.h>
#include <amqpprox_authrequestdata.h>

#include <iostream>

#include <boost/asio.hpp>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Performs authn/authz operations for incoming clients, implements the
 * AuthInterceptInterface interface
 */
class DefaultAuthIntercept : public AuthInterceptInterface {
  public:
    // CREATORS
    explicit DefaultAuthIntercept(boost::asio::io_service &ioService);

    virtual ~DefaultAuthIntercept() override = default;

    // MANIPULATORS
    /**
     * \brief It gets all the information required to authenticate from client
     * in authRequestData parameter and invoke callback function to provide
     * response.
     * \param authRequestData auth request data payload
     * \param responseCb Callbak function with response values
     */
    virtual void authenticate(const AuthRequestData    authRequestData,
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
