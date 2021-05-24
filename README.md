#  amqpprox
amqpprox is an AMQP 0.9.1 proxy server, it is designed for use in front of an
AMQP 0.9.1 compliant message queue broker such as RabbitMQ.

## Menu

- [Rationale](#rationale)
- [Quick start](#quick-start)
- [Building](#building)
- [Installation](#installation)
- [Code of Conduct](#code-of-conduct)
- [Contributions](#contributions)
- [License](#license)

## Rationale

Commonly people use HAProxy software acting as a load balancer to spread load
between multiple machines within a serving cluster and handle failures
gracefully. With amqpprox we built a similar proxy, except tailored
specifically for the AMQP 0.9.1 protocol. This brings benefits which cannot
be achieved by a layer 4 proxying alone.

### Key Advantages

1. Applying policies per vhost
2. Able to understand/log/replay AMQP sessions
3. Able to understand the downstream farm being a DNS resolve not single
   IP/port pairs
4. Detail statistics without relying on the RabbitMQ implementation
5. Ability to easily have clients test failovers locally

## Documentation

* [Overall architecture and code walkthrough](docs/architecture.md)
* [Session relationship between incoming and outgoing connections](docs/sessions.md)
* [How AMQP handshaking works in the proxy](docs/handshake.md)
* [Overview of the TLS options](docs/tls.md)

## Roadmap

#### Basic Features

- [X] Fast teardown/reestablishment: allow switching between farms per vhost
- [X] Load balancing between the farm members
- [ ] Resources (vhosts) can be pointed at DNS farms, not just IPs
- [ ] Periodic DNS refreshing

#### Advanced Features

- [ ] Traffic throttling/shaping per vhost/session/connection
- [ ] Load balancing that is aware of current queue location
- [ ] Automatic connection teardown/moving if IP leaves the farm
- [ ] Retry last frame to new connection on a failure
- [ ] Multiplexing multiple connections with multiple channels to existing
      connection + channels

#### Operational Features

- [X] Unix domain socket / pipe for control plane
- [X] No dependencies: config is pushed into it, does nothing by default
- [X] Be able to easily test client failover by severing connections on demand
- [ ] `SO_REUSEPORT` to scale on one box

#### Introspection Features

- [X] Maintain statistics about each connection/channel
- [ ] Be able to capture or trace selectively
- [ ] Be able to replay an interaction

## `amqpprox` & `amqpprox_ctl` Quick Start

`amqpprox` is the core proxy executable. It always starts up with no mapped vhosts/broker backends/listening ports/other configuration. `amqpprox_ctl` is used to provide config.

### Start `amqpprox`
```
$ amqpprox --help
amqpprox AMQP v0.9.1 proxy:
This is a proxy program for AMQP, designed to sit in front of a RabbitMQ
cluster. Most options for configuring the proxy and introspecting its
state are available through the amqpprox_ctl program, begin by sending
HELP to it. This program supports the following options to allow running
multiple instances on a machine:
  --help                               This help information
  --logDirectory arg (=logs)           Set logging directory
  --controlSocket arg (=/tmp/amqpprox) Set control UNIX domain socket location
  --cleanupIntervalMs arg (=1000)
$ amqpprox
Starting amqpprox, logging to: 'logs' control using: '/tmp/amqpprox'
```

### `amqpprox_ctl` Example Usage

```sh
$ amqpprox_ctl --help
Usage: amqpprox_ctl <control_socket> ARGS
```

`amqpprox` responds to HELP

```
$ amqpprox_ctl /tmp/amqpprox HELP
BACKEND (ADD name datacenter host port [SEND-PROXY] [TLS] | DELETE name | PRINT) - Change backend servers
CONN Print the connected sessions
DATACENTER SET name | PRINT
EXIT Exit the program gracefully.
FARM (ADD_DNS name dnsname port | ADD_MANUAL name selector backend* | PARTITION name policy | DELETE name | PRINT) - Change farms
HELP Print this help text.
LISTEN START port | START_SECURE port | STOP [port]
LOG CONSOLE verbosity | FILE verbosity
MAP (BACKEND vhost backend | FARM vhost name | UNMAP vhost | DEFAULT farmName | REMOVE_DEFAULT | PRINT) - Change mappings of resources to servers
MAPHOSTNAME DNS - Set up mapping of IPs to hostnames
SESSION  id# (PAUSE|DISCONNECT_GRACEFUL|FORCE_DISCONNECT) - Control a particular session
STAT (STOP SEND | SEND <host> <port> | (LISTEN (json|human) (overall|vhost=foo|backend=bar|source=baz|all|process|bufferpool)) - Output statistics
TLS (INGRESS | EGRESS) (KEY_FILE file | CERT_CHAIN_FILE file | RSA_KEY_FILE file | TMP_DH_FILE file | CA_CERT_FILE file | VERIFY_MODE mode*)
VHOST PAUSE vhost | UNPAUSE vhost | PRINT | BACKEND_DISCONNECT vhost | FORCE_DISCONNECT vhost
```

Configure `amqpprox` how to talk to an AMQP 0.9.1 backend called `rabbit1`, labelled as datacenter `london-az1`, running on `localhost:5672` without TLS/Proxy Protocol.
```
$ amqpprox_ctl /tmp/amqpprox BACKEND ADD rabbit1 london-az1 localhost 5672
$ amqpprox_ctl /tmp/amqpprox BACKEND PRINT
rabbit1 (london-az1): localhost 127.0.0.1:5672
```

Configure `amqpprox` to map inbound connections for `vhost-example` to backend `rabbit1`
```
$ amqpprox_ctl /tmp/amqpprox MAP BACKEND vhost-example rabbit1
$ amqpprox_ctl /tmp/amqpprox MAP PRINT
"vhost-example" => Backend:rabbit1
```

`amqpprox` now understands one vhost mapping. Use `LISTEN` to start listening for connections on port 5673:

```
$ amqpprox_ctl /tmp/amqpprox LISTEN START 5673
```

An AMQP 0.9.1 client can now connect to `vhost-example` on port 5673 and `amqpprox` will forward to `rabbit1` on port 5672.


## Building

The default build uses the Conan package manager, make, cmake and will require
a C++14 compiler. These should be installed prior to doing the native build,
and conan will download the other dependencies during setup.

Once you have set up the build dependencies on your machine, the project should
be buildable with the following commands:

1. `make setup` - This will initialise your Conan environment and create the build
   directory. It should only needs to be run the first time you are setting up
   a build, except when bumping the dependency versions.
2. `make init` - This will do the cmake initialisation, so should be run when
   changing the files included in the project, or the build system.
3. `make` - This will run an incremental build and unit-test

We also provide Dockerfiles for building and running in an isolated
environment, these are directly analogous to the above commands. If you don't
wish to get Conan set up on your build machine, this can be an alternative
quick way to get going.

1. `make docker-setup`
2. `make docker-init`
3. `make docker-build`
4. `make docker-shell`: Get an interactive shell within the build environment
   container.

There are a number of options exposed through the build system, they're
documented on the [buildfiles README](buildfiles/README.md).

We only recommend using Linux based operating systems for running amqpprox;
however, we also regularly develop it on Mac OS and expect changes to not break
the native build there.

## Installation

//TODO

## Contributions

We :heart: contributions.

Have you had a good experience with amqpprox? Why not share some love and contribute code, or just let us know about any issues you had with it?

We welcome issue reports [here](../../issues); be sure to choose the proper issue template for your issue, so that we can be sure you're providing the necessary information.

Before sending a [Pull Request](../../pulls), please make sure you read our
[Contribution Guidelines](https://github.com/bloomberg/.github/blob/master/CONTRIBUTING.md).

## License

Please read the [LICENSE](LICENSE) file.

## Code of Conduct

This project has adopted a [Code of Conduct](https://github.com/bloomberg/.github/blob/master/CODE_OF_CONDUCT.md).
If you have any concerns about the Code, or behavior which you have experienced in the project, please
contact us at opensource@bloomberg.net.

## Security Vulnerability Reporting

If you believe you have identified a security vulnerability in this project, please send email to the project
team at opensource@bloomberg.net, detailing the suspected issue and any methods you've found to reproduce it.

Please do NOT open an issue in the GitHub repository, as we'd prefer to keep vulnerability reports private until
we've had an opportunity to review and address them.
