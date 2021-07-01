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

#include <amqpprox_vhostestablishedpauser.h>

#include <amqpprox_eventsource.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>
#include <amqpprox_vhoststate.h>

namespace Bloomberg {
namespace amqpprox {

EventSubscriptionHandle vhostEstablishedPauser(EventSource *eventSource,
                                               Server *     server,
                                               VhostState * vhostState)
{
    return eventSource->connectionVhostEstablished().subscribe(
        [=](uint64_t id, const std::string &vhost) {
            if (vhostState->isPaused(vhost)) {
                auto session = server->getSession(id);
                session->pause();
            }
        });
}

}
}
