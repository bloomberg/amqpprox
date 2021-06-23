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
#include <amqpprox_eventsource.h>

namespace Bloomberg {
namespace amqpprox {

EventSource::EventSource()
: d_connectionReceived(ConnectionReceived::create())
, d_connectionVhostEstablished(ConnectionVhostEstablished::create())
, d_connectionEstablished(ConnectionEstablished::create())
, d_connectionFailed(ConnectionFailed::create())
, d_brokerConnectionSnapped(BrokerConnectionSnapped::create())
, d_clientConnectionSnapped(ClientConnectionSnapped::create())
, d_cleanDisconnect(CleanDisconnect::create())
, d_statisticsAvailable(StatisticsAvailable::create())
{
}

}
}
