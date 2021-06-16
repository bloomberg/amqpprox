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
#include <amqpprox_backendselectorstore.h>
#include <amqpprox_backendstore.h>
#include <amqpprox_bufferpool.h>
#include <amqpprox_control.h>
#include <amqpprox_cpumonitor.h>
#include <amqpprox_datacenter.h>
#include <amqpprox_eventsource.h>
#include <amqpprox_farmstore.h>
#include <amqpprox_frame.h>
#include <amqpprox_logging.h>
#include <amqpprox_loggingcontrolcommand.h>
#include <amqpprox_mappingconnectionselector.h>
#include <amqpprox_partitionpolicy.h>
#include <amqpprox_partitionpolicystore.h>
#include <amqpprox_resourcemapper.h>
#include <amqpprox_server.h>
#include <amqpprox_session.h>
#include <amqpprox_sessioncleanup.h>
#include <amqpprox_statcollector.h>
#include <amqpprox_vhostestablishedpauser.h>
#include <amqpprox_vhoststate.h>

// Backend selectors
#include <amqpprox_robinbackendselector.h>

// Partition policies
#include <amqpprox_affinitypartitionpolicy.h>

// Control commands
#include <amqpprox_backendcontrolcommand.h>
#include <amqpprox_connectionscontrolcommand.h>
#include <amqpprox_datacentercontrolcommand.h>
#include <amqpprox_exitcontrolcommand.h>
#include <amqpprox_farmcontrolcommand.h>
#include <amqpprox_helpcontrolcommand.h>
#include <amqpprox_listencontrolcommand.h>
#include <amqpprox_mapcontrolcommand.h>
#include <amqpprox_maphostnamecontrolcommand.h>
#include <amqpprox_sessioncontrolcommand.h>
#include <amqpprox_statcontrolcommand.h>
#include <amqpprox_tlscontrolcommand.h>
#include <amqpprox_vhostcontrolcommand.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <memory>
#include <thread>

namespace {
const char *HELP_TEXT = R"helptext(
amqpprox AMQP v0.9.1 proxy:

This is a proxy program for AMQP v0.9.1, designed to sit in front of a RabbitMQ cluster. Most options for configuring the proxy and introspecting its state are available through the amqpprox_ctl program, begin by sending 'HELP' to it.

This program supports the following options to allow running multiple instances on a machine and a simplified configuration mode. In the simplified configuration mode the --listenPort, --destinationDNS and --destinationPort must all be specified, and after which it immediately starts listening on all interfaces for that port and sends all vhosts to the destination DNS entry. More complicated configuration, such as sending different vhosts to different destinations, necessitates the use of the amqpprox_ctl.

Although most configuration is injected by the amqpprox_ctl program, the logging directories and the control UNIX domain socket are specified on this program, to facilitate safely running multiple instances of amqpprox on a single host.
)helptext";
}

// TODO update this HELP_TEXT

int main(int argc, char *argv[])
{
    using namespace std::string_literals;
    using namespace Bloomberg::amqpprox;
    namespace po = boost::program_options;

    std::string logDirectory;
    std::string controlSocket;
    uint32_t    cleanupIntervalMs;
    uint16_t    easyListenPort;
    uint16_t    easyDestinationPort;
    std::string easyDestinationDNS;
    uint16_t    consoleVerbosity;

    // Set up the basic command line options to allow multiple instances to run
    // on a single box without colliding
    po::options_description options(HELP_TEXT);
    options.add_options()("help", "This help information")(
        "logDirectory",
        po::value<std::string>(&logDirectory)->default_value("logs"),
        "Set logging directory")(
        "controlSocket",
        po::value<std::string>(&controlSocket)->default_value("/tmp/amqpprox"),
        "Set control UNIX domain socket location")(
        "cleanupIntervalMs",
        po::value<uint32_t>(&cleanupIntervalMs)->default_value(1000u),
        "Set the cleanup interval to garbage collect connections")(
        "listenPort",
        po::value<uint16_t>(&easyListenPort)->default_value(0),
        "Simple config mode: listening port")(
        "destinationPort",
        po::value<uint16_t>(&easyDestinationPort)->default_value(0),
        "Simple config mode: destination port")(
        "destinationDNS",
        po::value<std::string>(&easyDestinationDNS)->default_value(""),
        "Simple config mode: destination DNS address")(
        "consoleVerbosity,v",
        po::value<uint16_t>(&consoleVerbosity)->default_value(0),
        "Default console logging verbosity (0 = No output through to 5 = "
        "Trace-level)");

    po::variables_map variablesMap;

    try {
        po::store(po::parse_command_line(argc, argv, options), variablesMap);
    }
    catch (po::unknown_option &e) {
        std::cout << "Unknown command line option: " << e.get_option_name()
                  << "\n";
        std::cout << options << "\n";
        return 1;
    }

    po::notify(variablesMap);

    // If help has been requested we bail out early
    if (variablesMap.count("help")) {
        std::cout << options << "\n";
        return 1;
    }

    // Check if using simple configuration
    if ((easyListenPort != 0 || easyDestinationPort != 0 ||
         easyDestinationDNS != "") &&
        (easyListenPort == 0 || easyDestinationPort == 0 ||
         easyDestinationDNS == "")) {
        // All the easy options must be set at once
        std::cout
            << "If configuring in simple mode, the --listenPort, "
               "--destinationPort and --destinationDNS must all be set\n";
        return 2;
    }

    if (consoleVerbosity > 5) {
        std::cout << "Console log verbosity must be between 0 and 5\n";
        return 3;
    }

    Logging::start(logDirectory);

    std::cout << "Starting amqpprox, logging to: '" << logDirectory
              << "' control using: '" << controlSocket << "'" << std::endl;

    // Remove the control channel if it already existed, for example in the
    // case of a crashed process or machine
    ::unlink(controlSocket.c_str());

    // Buffer sizes in range, skipping some of the larger powers of 2 once we
    // get around page sizes.
    BufferPool     bufferPool({32,
                           64,
                           128,
                           256,
                           512,
                           1024,
                           4096,
                           16384,
                           32768,
                           65536,
                           Frame::getMaxFrameSize()});
    CpuMonitor     monitor;
    Datacenter     datacenter;
    EventSource    eventSource;
    FarmStore      farmStore;
    BackendStore   backendStore;
    ResourceMapper resourceMapper;
    StatCollector  statCollector;
    statCollector.setCpuMonitor(&monitor);
    statCollector.setBufferPool(&bufferPool);
    VhostState                vhostState;
    PartitionPolicyStore      partitionPolicyStore;
    BackendSelectorStore      backendSelectorStore;
    MappingConnectionSelector mappingSelector(
        &farmStore, &backendStore, &resourceMapper);
    SessionCleanup cleaner(&statCollector, &eventSource);

    Server  server(&mappingSelector, &eventSource, &bufferPool);
    Control control(&server, &eventSource, controlSocket);

    // Set up the backend selector store
    using BackendSelectorPtr       = std::unique_ptr<BackendSelector>;
    BackendSelectorPtr selectors[] = {
        BackendSelectorPtr(new RobinBackendSelector)};

    for (auto &&selector : selectors) {
        backendSelectorStore.addSelector(std::move(selector));
    }

    // Set up the policy store
    using PartitionPolicyPtr      = std::unique_ptr<PartitionPolicy>;
    PartitionPolicyPtr policies[] = {
        PartitionPolicyPtr(new AffinityPartitionPolicy(&datacenter))};

    for (auto &&policy : policies) {
        partitionPolicyStore.addPolicy(std::move(policy));
    }

    // Set up the control channel commands
    using CommandPtr      = std::unique_ptr<ControlCommand>;
    CommandPtr commands[] = {
        CommandPtr(new ExitControlCommand),
        CommandPtr(new ConnectionsControlCommand),
        CommandPtr(new HelpControlCommand),
        CommandPtr(new DatacenterControlCommand(&datacenter, &farmStore)),
        CommandPtr(new SessionControlCommand),
        CommandPtr(new FarmControlCommand(&farmStore,
                                          &backendStore,
                                          &backendSelectorStore,
                                          &partitionPolicyStore)),
        CommandPtr(new BackendControlCommand(&backendStore)),
        CommandPtr(new MapControlCommand(&resourceMapper, &mappingSelector)),
        CommandPtr(new VhostControlCommand(&vhostState)),
        CommandPtr(new ListenControlCommand),
        CommandPtr(new LoggingControlCommand),
        CommandPtr(new StatControlCommand(&eventSource)),
        CommandPtr(new MapHostnameControlCommand()),
        CommandPtr(new TlsControlCommand)};

    for (auto &&command : commands) {
        control.addControlCommand(std::move(command));
    }

    // Subscribe to vhost connections to set new connections to be paused once
    // the vhost is established late in the connection phase
    EventSubscriptionHandle vhostPauser =
        vhostEstablishedPauser(&eventSource, &server, &vhostState);

    // Schedule the cleanup task to run
    control.scheduleRecurringEvent(cleanupIntervalMs,
                                   "sessions-cleanup",
                                   std::bind(&SessionCleanup::cleanup,
                                             &cleaner,
                                             std::placeholders::_1,
                                             std::placeholders::_2));

    // Schedule the self-CPU monitor
    control.scheduleRecurringEvent(CpuMonitor::intervalMs(),
                                   "cpu-monitor",
                                   std::bind(&CpuMonitor::clock,
                                             &monitor,
                                             std::placeholders::_1,
                                             std::placeholders::_2));

    // Start the control thread separately
    std::thread controlThread([&]() { control.run(); });

    auto output = [](const std::string &output, bool finish) -> bool {
        std::cout << output;
        return true;
    };

    if (consoleVerbosity > 0) {
        control.getControlCommand("LOG")->handleCommand(
            "LOG",
            "CONSOLE "s + std::to_string(consoleVerbosity),
            output,
            &server,
            &control);
    }

    if (easyListenPort != 0) {
        control.getControlCommand("BACKEND")->handleCommand(
            "BACKEND",
            "ADD_DNS default-backend none "s + easyDestinationDNS + " " +
                std::to_string(easyDestinationPort),
            output,
            &server,
            &control);
        control.getControlCommand("FARM")->handleCommand(
            "FARM",
            "ADD default round-robin default-backend",
            output,
            &server,
            &control);
        control.getControlCommand("MAP")->handleCommand(
            "MAP", "DEFAULT default", output, &server, &control);
        control.getControlCommand("LISTEN")->handleCommand(
            "LISTEN",
            "START "s + std::to_string(easyListenPort),
            output,
            &server,
            &control);
    }

    // Start the server loop and block
    const int rc = server.run();
    controlThread.join();

    // Start tear down
    // Deinitialize the logging
    Logging::stop();

    // Clean up our own control socket
    ::unlink(controlSocket.c_str());

    std::cout << "Stopping amqpprox, instance: '" << controlSocket << "'"
              << std::endl;

    return rc;
}
