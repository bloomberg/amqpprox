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
#include <amqpprox_cpumonitor.h>

#include <sys/resource.h>

namespace Bloomberg {
namespace amqpprox {

CpuMonitor::CpuMonitor()
: d_currentCpuUser(0.0)
, d_currentCpuSystem(0.0)
, d_currentMaxRssKB(0)
, d_samples(0)
, d_mutex()
{
}

bool CpuMonitor::clock(Control * /* control */, Server * /* server */)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    boost::timer::cpu_times current_times = d_cpuTimer.elapsed();
    if (d_samples++) {
        d_currentCpuUser   = current_times.user / (1.0 * current_times.wall);
        d_currentCpuSystem = current_times.system / (1.0 * current_times.wall);
    }
    d_cpuTimer.start();

    struct rusage now;
    if (0 == getrusage(RUSAGE_SELF, &now)) {
        d_currentMaxRssKB = now.ru_maxrss;

#ifdef __MACH__
        // Darwin reports in bytes, linux in kilobytes ¯\_(ツ)_/¯
        d_currentMaxRssKB /= 1024;
#endif
    }

    return true;
}

CpuMonitor::UserSystemUsage CpuMonitor::currentCpu() const
{
    std::lock_guard<std::mutex> lock(d_mutex);
    return std::make_tuple(d_currentCpuUser, d_currentCpuSystem);
}

std::size_t CpuMonitor::currentRssKB() const
{
    std::lock_guard<std::mutex> lock(d_mutex);
    return d_currentMaxRssKB;
}

bool CpuMonitor::valid() const
{
    std::lock_guard<std::mutex> lock(d_mutex);
    return d_samples > 1;
}

int CpuMonitor::intervalMs()
{
    return 1000;
}

}
}
