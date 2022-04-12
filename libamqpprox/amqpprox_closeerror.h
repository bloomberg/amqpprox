/*
** Copyright 2022 Bloomberg Finance L.P.
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

#ifndef BLOOMBERG_AMQPPROX_CLOSEERROR
#define BLOOMBERG_AMQPPROX_CLOSEERROR

#include <amqpprox_methods_close.h>
#include <stdexcept>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Custom exception class
 *
 * This class will be used to throw custom exception, whenever we get the
 * unexpected connection Close method from server during handshake. Using this
 * exception, we are going to pass connection Close method to the exception
 * handler. The exception handler eventually can send the Close method to
 * clients for better error handling.
 */
class CloseError : public std::runtime_error {
    methods::Close d_closeMethod;

  public:
    // CREATORS
    CloseError(const std::string &msg, methods::Close &closeMethod);

    // ACCESSORS
    /**
     * \return connection Close method
     */
    methods::Close closeMethod() const;
};

}
}

#endif
