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
#ifndef BLOOMBERG_AMQPPROX_TLSUTIL
#define BLOOMBERG_AMQPPROX_TLSUTIL

#include <boost/asio/ssl.hpp>
#include <boost/system/error_code.hpp>

#include <openssl/ssl.h>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Util class for TLS communication
 */
class TlsUtil {
  public:
    /**
     * \brief Augment extra error information related to TLS based on specified
     * error_code
     * \param ec specifies error code
     * \return augmented error information
     */
    static std::string augmentTlsError(const boost::system::error_code &ec);

    /**
     * \brief Set information callback for SSL connections and also set the
     * callback used to verify peer certificates.
     * \param ctx ssl context
     */
    static void setupTlsLogging(boost::asio::ssl::context &ctx);

    /**
     * \brief Information callback function for SSL connections
     * \param s pointer to ssl_st struct defined in openssl/ssl.h
     * \param where specifies information about where (in which context) the
     * callback function was called
     * \param ret return error code
     */
    static void logTlsConnectionAlert(const SSL *s, int where, int ret);

    /**
     * \brief Callback function used to verify peer certificates
     * \param preverified represents certificate passed pre-verification
     * \param ctx the peer certificate and other context
     */
    static bool
    logCertVerificationFailure(bool                              preverified,
                               boost::asio::ssl::verify_context &ctx);
};

}  // namespace amqpprox
}  // namespace Bloomberg

#endif
