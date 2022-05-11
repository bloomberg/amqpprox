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
#ifndef BLOOMBERG_AMQPPROX_VHOSTESTABLISHEDPAUSER
#define BLOOMBERG_AMQPPROX_VHOSTESTABLISHEDPAUSER

#include <amqpprox_eventsourcesignal.h>

namespace Bloomberg {
namespace amqpprox {

class EventSource;
class Server;
class VhostState;

/**
 * \brief Subscribe to vhost connections to set new connections to be paused
 * once the vhost is established late in the connection phase
 * \param eventSource pointer to `EventSource`
 * \param server pointer to `Server`
 * \param vhostState pointer to `VhostState`
 */
EventSubscriptionHandle vhostEstablishedPauser(EventSource *eventSource,
                                               Server      *server,
                                               VhostState  *vhostState);

}
}

#endif
