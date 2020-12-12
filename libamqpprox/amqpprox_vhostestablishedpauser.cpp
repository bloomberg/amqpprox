/*
 * amqpprox_vhostestablishedpauser.cpp
 * Copyright (C) 2016 alaric <alaric@nyx.local>
 *
 * Distributed under terms of the MIT license.
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
