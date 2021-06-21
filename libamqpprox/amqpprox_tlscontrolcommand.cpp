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
#include <amqpprox_tlscontrolcommand.h>

#include <amqpprox_logging.h>
#include <amqpprox_server.h>

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <openssl/ssl.h>

namespace Bloomberg {
namespace amqpprox {
namespace {
void logCipherSuites(boost::asio::ssl::context &context, std::ostream &os)
{
    STACK_OF(SSL_CIPHER) *ciphers =
        SSL_CTX_get_ciphers(context.native_handle());

    for (size_t i = 0; i < sk_SSL_CIPHER_num(ciphers); i++) {
        const SSL_CIPHER *cipher = sk_SSL_CIPHER_value(ciphers, i);

        os << SSL_CIPHER_get_name(cipher) << "\n";
    }
}
}

TlsControlCommand::TlsControlCommand()
{
}

std::string TlsControlCommand::commandVerb() const
{
    return "TLS";
}

std::string TlsControlCommand::helpText() const
{
    return "(INGRESS | EGRESS) (KEY_FILE file | CERT_CHAIN_FILE file | "
           "RSA_KEY_FILE file | TMP_DH_FILE file | CA_CERT_FILE file | "
           "VERIFY_MODE mode*)";
}

void TlsControlCommand::handleCommand(const std::string & /* command */,
                                      const std::string &  restOfCommand,
                                      const OutputFunctor &outputFunctor,
                                      Server *             serverHandle,
                                      Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        direction, command, file;
    iss >> direction;
    iss >> command;

    boost::to_upper(direction);
    boost::to_upper(command);

    if (direction != "INGRESS" && direction != "EGRESS") {
        output << "Direction must be INGRESS or EGRESS\n";
        return;
    }

    auto &context = (direction == "INGRESS")
                        ? serverHandle->ingressTlsContext()
                        : serverHandle->egressTlsContext();
    boost::system::error_code ec;

    if ("VERIFY_MODE" == command) {
        boost::asio::ssl::verify_mode mode = 0;
        std::string                   mode_str;
        while (iss >> mode_str) {
            boost::to_upper(mode_str);

            if ("PEER" == mode_str) {
                mode |= boost::asio::ssl::verify_peer;
            }
            else if ("NONE" == mode_str) {
                mode |= boost::asio::ssl::verify_none;
            }
            else if ("FAIL_IF_NO_PEER_CERT" == mode_str) {
                mode |= boost::asio::ssl::verify_fail_if_no_peer_cert;
            }
            else if ("CLIENT_ONCE" == mode_str) {
                mode |= boost::asio::ssl::verify_client_once;
            }
            else {
                output << "Unknown mode: " << mode_str << "\n";
                return;
            }
        }

        context.set_verify_mode(mode, ec);

        if (ec) {
            output << "Error: " << ec;
        }

        return;
    }
    else if ("CIPHERS" == command) {
        std::string argument;
        iss >> argument;
        if ("PRINT" == argument) {
            output << "Ciphers:\n";
            logCipherSuites(context, output);
            return;
        }
        else if ("SET" == argument) {
            std::string cipherList;
            std::getline(iss, cipherList);

            int rc = SSL_CTX_set_cipher_list(context.native_handle(),
                                             cipherList.c_str());

            if (!rc) {
                output << "Failed to set cipher suite. ";
            }

            output << "New ciphers:\n";
            logCipherSuites(context, output);

            return;
        }
    }
    // All other commands operate on a single file argument

    iss >> file;

    if (file == "") {
        output << "File must be specified\n";
        return;
    }

    if ("KEY_FILE" == command) {
        context.use_private_key_file(file, boost::asio::ssl::context::pem, ec);
    }
    if ("CERT_CHAIN_FILE" == command) {
        context.use_certificate_chain_file(file, ec);
    }
    if ("RSA_KEY_FILE" == command) {
        context.use_rsa_private_key_file(
            file, boost::asio::ssl::context::pem, ec);
    }
    if ("TMP_DH_FILE" == command) {
        context.use_tmp_dh_file(file, ec);
    }
    if ("CA_CERT_FILE" == command) {
        context.load_verify_file(file, ec);
    }

    if (ec) {
        output << "Error: " << ec;
    }

    LOG_DEBUG << "Configured TLS: " << command << "=" << file;
}

}
}
