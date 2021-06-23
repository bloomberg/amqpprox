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
#include <amqpprox_tlsutil.h>

#include <amqpprox_logging.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <openssl/err.h>

namespace Bloomberg {
namespace amqpprox {

std::string TlsUtil::augmentTlsError(const boost::system::error_code &ec)
{
    std::string err = ec.message();

    if (ec.category() == boost::asio::error::get_ssl_category()) {
        err += " (" + std::to_string(ERR_GET_LIB(ec.value())) + "," +
               std::to_string(ERR_GET_FUNC(ec.value())) + "," +
               std::to_string(ERR_GET_REASON(ec.value())) + ") ";

        char tempBuf[128];
        ::ERR_error_string_n(ec.value(), tempBuf, sizeof(tempBuf));
        err += tempBuf;
    }

    return err;
}

void TlsUtil::logTlsConnectionAlert(const SSL *s, int where, int ret)
{
    if (where & SSL_CB_ALERT) {
        // Log any TLS alerts sent/received - can be "Unknown CA" etc.
        const char *prefix = (where & SSL_CB_READ) ? "read" : "write";
        LOG_ERROR << "SSL alert " << prefix << ": "
                  << SSL_alert_type_string_long(ret) << ": "
                  << SSL_alert_desc_string_long(ret);
    }
    else if (where & SSL_CB_EXIT) {
        // Log error/failure codes of SSL accept/connect calls
        const char *prefix  = "undefined";
        const int   st_type = where & ~SSL_ST_MASK;

        if (st_type & SSL_ST_CONNECT) {
            prefix = "SSL_connect";
        }
        else if (st_type & SSL_ST_ACCEPT) {
            prefix = "SSL_accept";

            if (ret == 0) {
                LOG_ERROR << prefix
                          << " failed in: " << SSL_state_string_long(s);
            }
            else if (ret < 0) {
                LOG_ERROR << prefix
                          << " error in: " << SSL_state_string_long(s);
            }
        }
    }
}

bool TlsUtil::logCertVerificationFailure(bool preverified,
                                         boost::asio::ssl::verify_context &ctx)
{
    if (!preverified) {
        char  subject_name[256];
        X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 255);

        LOG_ERROR << "Certificate verification failed: [" << subject_name
                  << "]: ";
    }

    return preverified;
}

void TlsUtil::setupTlsLogging(boost::asio::ssl::context &ctx)
{
    SSL_CTX_set_info_callback(ctx.native_handle(),
                              &TlsUtil::logTlsConnectionAlert);

    ctx.set_verify_callback(&TlsUtil::logCertVerificationFailure);
}

}  // namespace amqpprox
}  // namespace Bloomberg
