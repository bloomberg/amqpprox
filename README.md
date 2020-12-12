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

## Quick Start
 //TODO
 [ ] how to use

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
