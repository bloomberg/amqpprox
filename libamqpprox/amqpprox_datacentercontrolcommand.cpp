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
#include <amqpprox_datacentercontrolcommand.h>

#include <amqpprox_datacenter.h>
#include <amqpprox_farmstore.h>
#include <amqpprox_server.h>

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

DatacenterControlCommand::DatacenterControlCommand(Datacenter *datacenter,
                                                   FarmStore * farmStore)
: d_datacenter_p(datacenter)
, d_farmStore_p(farmStore)
{
}

std::string DatacenterControlCommand::commandVerb() const
{
    return "DATACENTER";
}

std::string DatacenterControlCommand::helpText() const
{
    return "SET name | PRINT";
}

void DatacenterControlCommand::handleCommand(
    const std::string & /* command */,
    const std::string &  restOfCommand,
    const OutputFunctor &outputFunctor,
    Server *             serverHandle,
    Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    if (iss >> subcommand) {
        boost::to_upper(subcommand);

        if (subcommand == "SET") {
            std::string name;
            iss >> name;
            d_datacenter_p->set(std::move(name));

            // Repartition all farms
            d_farmStore_p->repartitionAll();
        }
        else if (subcommand == "PRINT") {
            output << d_datacenter_p->get() << "\n";
        }
        else {
            output << "Unknown subcommand.\n";
        }
    }
    else {
        output << "No subcommand provided.\n";
    }
}

}
}
