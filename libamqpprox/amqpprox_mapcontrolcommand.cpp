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
#include <amqpprox_mapcontrolcommand.h>

#include <amqpprox_mappingconnectionselector.h>
#include <amqpprox_resourcemapper.h>

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <string>

namespace Bloomberg {
namespace amqpprox {

MapControlCommand::MapControlCommand(ResourceMapper            *mapper,
                                     MappingConnectionSelector *selector)
: d_mapper_p(mapper)
, d_selector_p(selector)
{
}

std::string MapControlCommand::commandVerb() const
{
    return "MAP";
}

std::string MapControlCommand::helpText() const
{
    return "("
           "BACKEND vhost backend | "
           "FARM vhost name | "
           "UNMAP vhost | "
           "DEFAULT farmName | "
           "REMOVE_DEFAULT | "
           "PRINT"
           ") - Change mappings of resources to servers";
}

void MapControlCommand::handleCommand(const std::string & /* command */,
                                      const std::string   &restOfCommand,
                                      const OutputFunctor &outputFunctor,
                                      Server * /* serverHandle */,
                                      Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subCommand;

    iss >> subCommand;
    boost::to_upper(subCommand);

    if (subCommand == "BACKEND") {
        std::string vhost;
        std::string backendName;

        if (iss >> vhost && iss >> backendName) {
            d_mapper_p->mapVhostToBackend(vhost, backendName);
        }
        else {
            output << "Vhost and backend must be provided";
        }
    }
    else if (subCommand == "FARM") {
        std::string vhost;
        std::string farmName;

        if (iss >> vhost && iss >> farmName) {
            d_mapper_p->mapVhostToFarm(vhost, farmName);
        }
        else {
            output << "Vhost and farm name must be provided";
        }
    }
    else if (subCommand == "UNMAP") {
        std::string vhost;

        if (iss >> vhost) {
            d_mapper_p->unmapVhost(vhost);
        }
        else {
            output << "Vhost not provided.";
        }
    }
    else if (subCommand == "DEFAULT") {
        std::string farmName{""};
        if (iss >> farmName) {
            d_selector_p->setDefaultFarm(farmName);
        }
        else {
            output << "Farm name for default must be provided";
        }
    }
    else if (subCommand == "REMOVE_DEFAULT") {
        d_selector_p->unsetDefaultFarm();
    }
    else if (subCommand == "PRINT") {
        d_mapper_p->print(output);
    }
    else {
        output << "Unrecognized subcommand.";
    }
}

}
}
