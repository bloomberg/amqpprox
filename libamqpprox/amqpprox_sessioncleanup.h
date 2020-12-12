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
#ifndef BLOOMBERG_AMQPPROX_SESSIONCLEANUP
#define BLOOMBERG_AMQPPROX_SESSIONCLEANUP

namespace Bloomberg {
namespace amqpprox {

class Control;
class EventSource;
class Server;
class StatCollector;

/* \brief Performs periodic cleanup over the sessions
 *
 * This class performs periodic cleanup and stat collection from all of the
 * sessions in a `Server`. It is designed to be invoked on a periodicity of the
 * sessions needing to be cleaned up, this also dictates the periodicity that
 * the stats are outputted.
 */
class SessionCleanup {
    StatCollector *d_statCollector_p;  // HELD NOT OWNED
    EventSource *  d_eventSource_p;    // HELD NOT OWNED

  public:
    // CREATORS
    SessionCleanup(StatCollector *statCollector, EventSource *eventSource);
    ///< Construct a `SessionCleanup` object and take references to the
    ///< `StatCollector` and `EventSource` for storing session metrics in
    ///< and notifying that metrics are available for consumption

    // MANIPULATORS
    bool cleanup(Control *control, Server *server);
    ///< Run the cleanup of sessions on the given `Server` object and
    ///< returns a boolean indicating successful completion. This is
    ///< designed to operate with the recurring event scheduling in
    ///< `Control`, which uses a `true` return to indicate scheduling the
    ///< event again.
};

}
}

#endif
