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

#ifndef BLOOMBERG_AMQPPROX_CONNECTORUTIL
#define BLOOMBERG_AMQPPROX_CONNECTORUTIL

#include <amqpprox_fieldtable.h>
#include <amqpprox_methods_start.h>
#include <amqpprox_methods_startok.h>
#include <amqpprox_methods_tune.h>

#include <string_view>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Utilities for the Connector class to generate AMQ protocol items
 *
 * This component provides static functions that can be shared between the
 * primary user, the Connector class and any test components.
 */
class ConnectorUtil {
    /**
     * \brief Generate the server property bag.
     * \return Server properties field table
     *
     * This will have a similar set of capabilities as the RabbitMQ broker, but
     * contain the proxy's information.
     */
    static FieldTable generateServerProperties();

  public:
    /**
     * \brief Construct the start method the proxy will send
     * \return Start method
     */
    static methods::Start synthesizedStart();

    /**
     * \brief Construct the tune method the proxy will send
     * \return Tune method
     */
    static methods::Tune synthesizedTune();

    /**
     * \brief Mutate the StartOk from the client to include proxy information.
     * \param startOk output parameter for method to mutate
     * \param hostname the IP/hostname of the client of the proxy
     * \param port the TCP port of the client of the proxy
     * \param localHostname Local hostname
     * \param inboundListenPort Inbound listen port
     * \param outboundListenPort Outbound listen port
     * \param isIngressSecured Represents ingress connection is secured (TLS
     * enabled)
     */
    static void injectProxyClientIdent(methods::StartOk * startOk,
                                       const std::string &clientHostname,
                                       int                clientRemotePort,
                                       std::string_view   localHostname,
                                       int                inboundListenPort,
                                       int                outboundLocalPort,
                                       bool               isIngressSecured);
};

}
}

#endif
