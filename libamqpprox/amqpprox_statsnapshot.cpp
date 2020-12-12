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
#include <amqpprox_statsnapshot.h>

namespace Bloomberg {
namespace amqpprox {

StatSnapshot::StatSnapshot()
: d_vhosts()
, d_sources()
, d_backends()
, d_overallConnectionStats()
, d_process()
, d_pool()
, d_poolSpillover(0)
{
}

void StatSnapshot::swap(StatSnapshot &rhs)
{
    d_vhosts.swap(rhs.d_vhosts);
    d_sources.swap(rhs.d_sources);
    d_backends.swap(rhs.d_backends);
    d_overallConnectionStats.swap(rhs.d_overallConnectionStats);
    d_pool.swap(rhs.d_pool);

    StatSnapshot::ProcessStats temp = d_process;
    d_process                       = rhs.d_process;
    rhs.d_process                   = temp;

    std::swap(d_poolSpillover, rhs.d_poolSpillover);
}

bool operator==(const StatSnapshot::ProcessStats &lhs,
                const StatSnapshot::ProcessStats &rhs)
{
    return lhs.d_user == rhs.d_user && lhs.d_system == rhs.d_system &&
           lhs.d_overall == rhs.d_overall && lhs.d_rssKB == rhs.d_rssKB;
}

bool operator!=(const StatSnapshot::ProcessStats &lhs,
                const StatSnapshot::ProcessStats &rhs)
{
    return !(lhs == rhs);
}

}
}
