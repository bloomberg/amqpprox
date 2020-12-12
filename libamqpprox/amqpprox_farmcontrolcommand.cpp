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
#include <amqpprox_farmcontrolcommand.h>

#include <amqpprox_backendselector.h>
#include <amqpprox_backendselectorstore.h>
#include <amqpprox_backendstore.h>
#include <amqpprox_farm.h>
#include <amqpprox_farmstore.h>
#include <amqpprox_logging.h>
#include <amqpprox_partitionpolicy.h>
#include <amqpprox_partitionpolicystore.h>

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

namespace Bloomberg {
namespace amqpprox {

FarmControlCommand::FarmControlCommand(
    FarmStore *           store,
    BackendStore *        backendStore,
    BackendSelectorStore *backendSelectorStore,
    PartitionPolicyStore *partitionPolicyStore)
: d_store_p(store)
, d_backendStore_p(backendStore)
, d_backendSelectorStore_p(backendSelectorStore)
, d_partitionPolicyStore_p(partitionPolicyStore)
{
}

std::string FarmControlCommand::commandVerb() const
{
    return "FARM";
}

std::string FarmControlCommand::helpText() const
{
    return "("
           "ADD_DNS name dnsname port | "
           "ADD_MANUAL name selector backend* | "
           "PARTITION name policy | "
           "DELETE name | "
           "PRINT"
           ") - Change farms";
}

void FarmControlCommand::handleCommand(const std::string & /* command */,
                                       const std::string &  restOfCommand,
                                       const OutputFunctor &outputFunctor,
                                       Server * /* serverHandle */,
                                       Control * /* controlHandle */)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    iss >> subcommand;
    boost::to_upper(subcommand);

    if (subcommand == "ADD_DNS") {
        std::string    name;
        std::string    dnsName;
        unsigned short port;
        iss >> name;
        iss >> dnsName;
        iss >> port;
        if (name.empty() || dnsName.empty() || !port) {
            output << "Name, DNS address and port must be specified";
        }
        else {
            std::unique_ptr<Farm> farmPtr(new Farm(name, dnsName, port));
            d_store_p->addFarm(std::move(farmPtr));
        }
    }
    else if (subcommand == "ADD_MANUAL") {
        std::string              name;
        std::string              backendSelector;
        std::vector<std::string> backends;
        iss >> name;
        iss >> backendSelector;

        if (!iss.good() || name.empty() || backendSelector.empty()) {
            output << "Farm name and selector must be provided.\n";
            return;
        }

        BackendSelector *selector =
            d_backendSelectorStore_p->getSelector(backendSelector);
        if (selector == nullptr) {
            output << "Selector '" << backendSelector << "' not found\n";
            return;
        }

        bool allBackendsOk = true;
        while (iss.good() && !iss.eof()) {
            std::string backendName;
            iss >> backendName;

            auto backend = d_backendStore_p->lookup(backendName);
            if (!backend) {
                allBackendsOk = false;
                output << "Backend '" << backendName << "' not found\n";
            }

            backends.emplace_back(backendName);
        }

        if (allBackendsOk) {
            std::unique_ptr<Farm> farmPtr(
                new Farm(name, backends, d_backendStore_p, selector));
            d_store_p->addFarm(std::move(farmPtr));
        }
        else {
            output
                << "Farm not inserted due to at least one missing Backend\n";
        }
    }
    else if (subcommand == "PARTITION") {
        std::string name;
        std::string partitionPolicy;
        iss >> name;
        iss >> partitionPolicy;

        if (name.empty() || partitionPolicy.empty()) {
            output << "Farm name and policy must be provided.\n";
            return;
        }

        PartitionPolicy *policy =
            d_partitionPolicyStore_p->getPolicy(partitionPolicy);
        if (policy == nullptr) {
            output << "Partition policy '" << partitionPolicy
                   << "' not found\n";
            return;
        }

        try {
            Farm &farm = d_store_p->getFarmByName(name);
            farm.addPartitionPolicy(policy);
        }
        catch (std::runtime_error &e) {
            output << "Farm '" << name << "' not found\n";
            return;
        }
    }
    else if (subcommand == "DELETE") {
        std::string name;
        iss >> name;
        if (name.empty()) {
            output << "Farm name not provided.";
        }
        else {
            d_store_p->removeFarmByName(name);
        }
    }
    else if (subcommand == "PRINT") {
        std::ostringstream oss;
        d_store_p->print(output);
    }
    else {
        output << "Subcommand '" << subcommand << "' not recognized.";
    }
}

}
}
