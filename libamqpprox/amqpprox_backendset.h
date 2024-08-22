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
#ifndef BLOOMBERG_AMQPPROX_BACKENDSET
#define BLOOMBERG_AMQPPROX_BACKENDSET

#include <amqpprox_backend.h>

#include <cstdint>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Represents all the available `Backend` instances that can be used for
 * particular session.
 *
 * It partitions the available backends into priority-ordered groups. That is,
 * items in the first partition will all be equally preferred to all items in
 * the second partition, which will all be preferred over those in the third,
 * and so on.
 */
class BackendSet {
  public:
    // CLASS TYPES
    using Partition = std::vector<const Backend *>;
    using Marker    = uint64_t;

  private:
    // DATA
    std::vector<Partition> d_partitions;
    std::vector<Marker>    d_markers;

  public:
    // CREATORS
    /**
     * \brief Create a `BackendSet` containing the specified, ordered
     * partitions. The set of partitions contained within this instance is
     * immutable.
     * \param partitions Partitions vector
     */
    explicit BackendSet(std::vector<Partition> partitions);

    // MANIPULATORS
    /**
     * \brief Mark the specified `partitionId` as accessed
     * \param partitionId Partition ID
     * \return New value of the marker
     *
     * This will move the partition's marker. The behaviour is undefined unless
     * `partitionId` is within the range of the number of markers for this
     * instance.
     */
    uint64_t markPartition(uint64_t partitionId);

    // ACCESSORS
    /**
     * \return Non-modifiable reference to the vector of `Partitions` stored in
     * this `BackendSet` instance.
     */
    const std::vector<Partition> &partitions() const;

    /**
     * \return Non-modifiable reference to the vector of markers stored in this
     * `BackendSet` instance.
     */
    const std::vector<Marker> &markers() const;
};

// ACCESSORS
inline const std::vector<BackendSet::Partition> &BackendSet::partitions() const
{
    return d_partitions;
}

inline const std::vector<BackendSet::Marker> &BackendSet::markers() const
{
    return d_markers;
}

}
}

#endif
