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
#ifndef BLOOMBERG_AMQPPROX_CONNECTIONMANAGER
#define BLOOMBERG_AMQPPROX_CONNECTIONMANAGER

#include <amqpprox_backendset.h>

#include <memory>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class Backend;
class BackendSelector;

/*
 * A `ConnectionManager` represents an ongoing attempt to open an outgoing
 * connection for a given `Session`.
 *
 * An instance of `ConnectionManager` is created when a `Session` instance
 * attempts to open a connection to a broker on behalf of a client of the
 * proxy. The `getConnection` method is then invoked repeatedly with varying
 * `retryCount` values, as required, until the connection is successfully
 * established or there are no further connections to attempt.
 *
 * The manager tracks three data points: a `BackendSet` representing all the
 * available `Backend` instances that can be used for this session, a snapshot
 * of the `Marker` state of the `BackendSet` at the point the connection
 * attempt was started, and the `BackendSelector` that should be used to
 * choose one of the available `Backend` instances in the set. This data is
 * used to issue `Backend` instances to the caller in a well-defined order,
 * in which it will attempt to open outgoing connections.
 */
class ConnectionManager {
  private:
    // DATA
    std::shared_ptr<BackendSet>     d_backendSet;
    std::vector<BackendSet::Marker> d_markerSnapshot;
    BackendSelector *               d_backendSelector_p;  // HELD NOT OWNED

  public:
    // CREATORS
    /**
     * \brief Create a `ConnectionManager` instance backed by the specified
     * `backendSet` and `backendSelector`. The `Marker` value of the specified
     * `backendSet` will be snapshotted at construction time.
     *
     * \param backendSet Backend set
     * \param backendSelector Backend selector
     */
    ConnectionManager(std::shared_ptr<BackendSet> backendSet,
                      BackendSelector *           backendSelector);
    ///< Create a `ConnectionManager` instance backed by the specified
    ///< `backendSet` and `backendSelector`.
    ///< The `Marker` value of the specified `backendSet` will be
    ///< snapshotted at construction time.

    // ACCESSORS
    /**
     * \return `BackendSet` backing this instance, from which `Backend`
     * connection candidates will be drawn by the selector.
     */
    const std::shared_ptr<BackendSet> &backendSet() const;

    /**
     * \return const reference to the `Marker` snapshot that was taken of the
     * `BackendSet` at the point this instance was created
     */
    const std::vector<BackendSet::Marker> &markerSnapshot() const;

    /**
     * \return pointer to the `BackendSelector` instance being used by this
     * instance to select `Backend` connection candidates for the caller
     */
    BackendSelector *backendSelector() const;

    /**
     * \brief Return a pointer to a `Backend` connection candidate, to which an
     * outgoing connection should be attempted.
     * \param retryCount Retry count
     * \return pointer to a `Backend` connection candidate or `nullptr`
     *
     * If an outgoing connection attempt is unsuccessful, this method should be
     * re-called with an incremented `retryCount`. The possible candidates are
     * defined by the `BackendSet` backing this instance. The order in which
     * the candidates will be returned is defined by the `BackendSelector` and
     * `Marker` snapshot. If there are no valid `Backend` instances to connect
     * to, this method will return `nullptr`.
     */
    const Backend *getConnection(uint64_t retryCount) const;
};

// ACCESSORS
inline const std::shared_ptr<BackendSet> &ConnectionManager::backendSet() const
{
    return d_backendSet;
}

inline const std::vector<BackendSet::Marker> &
ConnectionManager::markerSnapshot() const
{
    return d_markerSnapshot;
}

inline BackendSelector *ConnectionManager::backendSelector() const
{
    return d_backendSelector_p;
}

}
}

#endif
