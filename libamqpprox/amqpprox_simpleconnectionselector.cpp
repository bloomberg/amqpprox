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
#include <amqpprox_simpleconnectionselector.h>

#include <amqpprox_connectionmanager.h>
#include <amqpprox_logging.h>
#include <amqpprox_sessionstate.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

SimpleConnectionSelector::SimpleConnectionSelector()
: d_selector()
, d_currentIndex(0)
{
}

int SimpleConnectionSelector::acquireConnection(
    std::shared_ptr<ConnectionManager> *connectionOut,
    const SessionState &                state)
{
    Backend backend1("backend", "dc1", "localhost", "127.0.0.1", 5672);
    Backend backend2("backend", "dc1", "localhost", "127.0.0.1", 5673);
    Backend backend3("backend", "dc1", "localhost", "127.0.0.1", 5674);

    std::vector<BackendSet::Partition> partitions;
    partitions.push_back(
        BackendSet::Partition{&backend1, &backend2, &backend3});

    *connectionOut = std::make_shared<ConnectionManager>(
        std::make_shared<BackendSet>(partitions), &d_selector);

    return 0;
}

}
}
