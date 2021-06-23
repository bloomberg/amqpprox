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
#ifndef BLOOMBERG_AMQPPROX_BACKENDSET
#define BLOOMBERG_AMQPPROX_BACKENDSET

#include <amqpprox_backend.h>

#include <vector>

namespace Bloomberg {
namespace amqpprox {

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
    explicit BackendSet(std::vector<Partition> partitions);
    ///< Create a `BackendSet` containing the specified, ordered
    ///< partitions. The set of partitions contained within this instance
    ///< is immutable.

    // MANIPULATORS
    uint64_t markPartition(uint64_t partitionId);
    ///< Mark the specified `partitionId` as accessed. This will move the
    ///< partition's marker. Returns the new value of the marker.
    ///< The behaviour is undefined unless `partitionId` is within the
    ///< range of the number of markers for this instance.

    // ACCESSORS
    const std::vector<Partition> &partitions() const;
    ///< Return a non-modifiable reference to the vector of `Partitions`
    ///< stored in this `BackendSet` instance.

    const std::vector<Marker> &markers() const;
    ///< Return a non-modifiable reference to the vector of markers stored
    ///< in this `BackendSet` instance.
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
