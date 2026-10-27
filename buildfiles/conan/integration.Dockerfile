# Build the Rust performance tester in its own stage so the final image does
# not need a toolchain installed via `curl | sh`.
FROM rust:1.99-slim-bookworm@sha256:86234e857a413004348b471d11a4ebe1aa92d2eae2b6b143cac68fd1ac3c48be AS perf_tester_build
COPY tests/performance_tester /perf_tester
RUN cargo build --release --locked --manifest-path /perf_tester/Cargo.toml

FROM rabbitmq:4.3.6@sha256:3e5cc23235b0eb850f604e8ace10d9a9b6d05be3e36730e53522ee9ead707a9e

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for integration tests
RUN apt-get update && apt-get dist-upgrade -y && \
    apt-get install -y python3 python3-pip python3-venv \
    curl llvm make cmake build-essential npm
COPY buildfiles/conan/requirements.txt buildfiles/conan/integration-requirements.txt /tmp/
# Use a venv so the hash-pinned versions don't clash with Debian-installed
# Python packages (e.g. python3-yaml) pulled in by the apt dependencies above.
RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --require-hashes -r /tmp/integration-requirements.txt
ENV PATH="/opt/venv/bin:${PATH}"

# Configure RabbitMQ for tests: enable file logging, disable prometheus
# (prometheus plugin binds a fixed port, conflicts when running multiple nodes)
RUN mkdir -p /etc/rabbitmq /tmp/logs && \
    printf 'log.file.level = info\nprometheus.tcp.port = 0\n' > /etc/rabbitmq/rabbitmq.conf && \
    /opt/rabbitmq/sbin/rabbitmq-plugins disable --offline rabbitmq_prometheus

EXPOSE 15800
EXPOSE 15801

ENV BUILDDIR=/build
ENV CONAN_USER_HOME=/build

COPY . /source
WORKDIR /source/tests/acceptance/integration
RUN npm ci
WORKDIR /source

RUN make setup && make init && make
COPY --from=perf_tester_build /perf_tester/target/release/amqpprox_perf_tester /usr/local/bin/
ENV PERF_TESTER_BIN=/usr/local/bin/amqpprox_perf_tester
ENV ROBOT_SOURCE_DIR=/source/tests/acceptance ROBOT_BINARY_DIR=/opt/rabbitmq/sbin
ENV AMQPPROX_BIN_DIR=/build/bin
ENV NODE_OPTIONS=--dns-result-order=ipv4first
