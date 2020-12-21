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

#include <amqpprox_connectorutil.h>

#include <amqpprox_constants.h>

#include <sstream>
#include <string_view>

namespace Bloomberg {
namespace amqpprox {

methods::Tune ConnectorUtil::synthesizedTune()
{
    return methods::Tune(Constants::channelMaximum(),
                         Constants::maxFrameSize(),
                         Constants::defaultHeartbeatInterval());
}

methods::Start ConnectorUtil::synthesizedStart()
{
    return methods::Start(Constants::versionMajor(),
                          Constants::versionMinor(),
                          generateServerProperties(),
                          {Constants::authenticationMechanism()},
                          {Constants::locale()});
}

FieldTable ConnectorUtil::generateServerProperties()
{
    // TODO Enhancement: Some of this could be injected from config. At the
    // moment, if RabbitMQ adds any extra capabilities we have to reflect them
    // here once all the serving RabbitMQ brokers support them. This is because
    // we have no way to discover/store them for the target broker, and no way
    // to know what the target broker is until later in the protocol.
    FieldTable ft;

    auto capabilitiesTable = std::make_shared<FieldTable>();
    auto capabilities      = {"publisher_confirms",
                         "exchange_exchange_bindings",
                         "basic.nack",
                         "consumer_cancel_notify",
                         "connection.blocked",
                         "consumer_priorities",
                         "authentication_failure_close",
                         "per_consumer_qos",
                         "direct_reply_to"};

    for (const auto &cap : capabilities) {
        capabilitiesTable->pushField(cap, FieldValue('t', true));
    }

    ft.pushField("capabilities", FieldValue('F', capabilitiesTable));
    ft.pushField("cluster_name",
                 FieldValue('S', std::string(Constants::clusterName())));
    ft.pushField("copyright",
                 FieldValue('S', std::string(Constants::copyrightNotice())));
    ft.pushField("product",
                 FieldValue('S', std::string(Constants::product())));
    ft.pushField("version",
                 FieldValue('S', std::string(Constants::version())));

    return ft;
}

void ConnectorUtil::injectProxyClientIdent(methods::StartOk * startOk,
                                           const std::string &clientHostname,
                                           int                clientRemotePort,
                                           std::string_view   localHostname,
                                           int outboundLocalPort)
{
    std::stringstream remoteClient;
    remoteClient << clientHostname << ":" << clientRemotePort;
    startOk->properties().pushField("amqpprox_client",
                                    FieldValue('S', remoteClient.str()));

    std::stringstream proxyInfo;
    proxyInfo << localHostname << ":" << outboundLocalPort;
    startOk->properties().pushField("amqpprox_host",
                                    FieldValue('S', proxyInfo.str()));
}

}
}
