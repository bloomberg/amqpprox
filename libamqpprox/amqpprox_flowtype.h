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
#ifndef BLOOMBERG_AMQPPROX_FLOWTYPE
#define BLOOMBERG_AMQPPROX_FLOWTYPE

#include <iosfwd>

namespace Bloomberg {
namespace amqpprox {

/* \brief Which side of the proxy is being serviced with a particular flow
 *
 * This describes the direction of the data flow with respect to the proxy.
 * The `INGRESS` direction is for flows originating from the proxies'
 * clients and `EGRESS` is for data originating (ie pushed) from the broker.
 * Note that AMQP is in most cases fully bi-directional, so there's no clear
 * request-response linkage between data flows. As such, most interactions in
 * the proxy are symmetric and pass-through but initiated by either the broker
 * or the client, and this identifies which direction the packets came from.
 *
 *     ----------          ---------          -------------------
 *     |        | INGRESS  |       |  EGRESS  |                 |
 *     | Client |<-------->| Proxy |<-------->| RabbitMQ Broker |
 *     |        |          |       |          |                 |
 *     ----------          ---------          -------------------
 */
enum class FlowType { INGRESS = 1, EGRESS };

std::ostream &operator<<(std::ostream &os, FlowType rhs);

}
}

#endif
