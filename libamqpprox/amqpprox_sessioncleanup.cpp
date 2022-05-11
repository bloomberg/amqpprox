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
#include <amqpprox_sessioncleanup.h>

#include <amqpprox_eventsource.h>
#include <amqpprox_logging.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>
#include <amqpprox_statcollector.h>

namespace Bloomberg {
namespace amqpprox {

SessionCleanup::SessionCleanup(StatCollector *statCollector,
                               EventSource   *eventSource)
: d_statCollector_p(statCollector)
, d_eventSource_p(eventSource)
{
}

bool SessionCleanup::cleanup(Control *control, Server *server)
{
    LOG_TRACE << "Session cleanup starting";

    std::vector<std::shared_ptr<Session>> sessionsDeleted;

    // Visit each of the sessions retrieving the statistics and setting any
    // sessions that are disconnected to be deleted.
    auto visitor = [this, server, &sessionsDeleted](
                       const std::shared_ptr<Session> &session) {
        d_statCollector_p->collect(session->state());

        if (session->finished()) {
            LOG_TRACE << "Cleaning session: " << session->state();
            sessionsDeleted.push_back(session);
        }
    };

    server->visitSessions(visitor);

    // Broadcast collected stats to all listeners
    d_eventSource_p->statisticsAvailable().emit(d_statCollector_p);

    std::size_t numberToDelete = sessionsDeleted.size();
    if (numberToDelete > 0) {
        // Here we tell the stat collector to account for the session being
        // deleted, this prevents the counters from it being involved in any
        // future stats operations, we then mark it as removed in the `Server`.
        for (auto &session : sessionsDeleted) {
            d_statCollector_p->deletedSession(session->state());
            server->removeSession(session->state().id());
        }

        // This releases the references for the sessions from the control
        // thread running the cleanup job, which should leave references only
        // from the server io loop thread
        sessionsDeleted.clear();

        // Here we actually schedule all the removed sessions to have their
        // last reference from the `Server` removed on the `Server` event loop.
        // NB: that this must occur after this thread relinquishes the
        // references, incase the `Server`'s reference was the last.
        server->clearDefunctSessions();
        LOG_INFO << "Cleaned up " << numberToDelete << " sessions.";
    }
    else {
        LOG_TRACE << "Clean up finished with no sessions to clean up";
    }

    // Reset the statistics ready for the next iteration
    d_statCollector_p->reset();
    return true;
}

}
}
