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
#include <amqpprox_backendcontrolcommand.h>

#include <amqpprox_backendstore.h>
#include <amqpprox_constants.h>
#include <amqpprox_control.h>

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

namespace Bloomberg {
namespace amqpprox {

BackendControlCommand::BackendControlCommand(BackendStore *store)
: d_store_p(store)
{
}

std::string BackendControlCommand::commandVerb() const
{
    return "BACKEND";
}

std::string BackendControlCommand::helpText() const
{
    return "(ADD name datacenter host port [SEND-PROXY] [TLS] | ADD_DNS name "
           "datacenter address port [SEND-PROXY] [TLS] | DELETE name | "
           "PRINT) - Change backend servers";
}

void BackendControlCommand::handleCommand(const std::string & /* command */,
                                          const std::string &  restOfCommand,
                                          const OutputFunctor &outputFunctor,
                                          Server * /* serverHandle */,
                                          Control *controlHandle)
{
    ControlCommandOutput<OutputFunctor> output(outputFunctor);

    std::istringstream iss(restOfCommand);
    std::string        subcommand;
    iss >> subcommand;
    boost::to_upper(subcommand);

    if (subcommand == "ADD" || subcommand == "ADD_DNS") {
        std::string name;
        std::string datacenter;
        std::string host;
        int         port = 0;
        std::string arg1, arg2;
        iss >> name;
        iss >> datacenter;
        iss >> host;
        iss >> port;
        iss >> arg1;
        iss >> arg2;
        boost::to_upper(arg1);
        boost::to_upper(arg2);

        if (!name.empty() && !datacenter.empty() && !host.empty() && port) {
            auto &ioService = controlHandle->ioService();
            boost::asio::ip::tcp::resolver        resolver(ioService);
            boost::asio::ip::tcp::resolver::query q(host, "");
            boost::system::error_code             ec;
            auto                                  it = resolver.resolve(q, ec);
            if (ec) {
                output << "Failed to resolve '" << host
                       << "', error code: " << ec;
                return;
            }

            std::string ip          = it->endpoint().address().to_string();
            bool        isSendProxy = arg1 == Constants::sendProxy() ||
                               arg2 == Constants::sendProxy();
            bool isSecure = arg1 == Constants::tlsCommand() ||
                            arg2 == Constants::tlsCommand();
            bool    isDns = subcommand == "ADD_DNS";
            Backend b(name,
                      datacenter,
                      host,
                      ip,
                      port,
                      isSendProxy,
                      isSecure,
                      isDns);

            int rc = d_store_p->insert(b);
            if (rc) {
                output << "Failed to insert backend: '" << b
                       << "', error code: " << rc;
                return;
            }
        }
        else {
            output << "Arguments not correctly provided";
        }
    }
    else if (subcommand == "DELETE") {
        std::string name;
        iss >> name;
        if (name.empty()) {
            output << "DELETE requires a name argument";
        }
        else {
            int rc = d_store_p->remove(name);
            if (rc) {
                output << "Delete failed to remove '" << name << "', rcode "
                       << rc;
            }
        }
    }
    else if (subcommand == "PRINT") {
        d_store_p->print(output);
    }
    else {
        output << "Subcommand '" << subcommand << "' not recognized.";
    }
}

}
}
