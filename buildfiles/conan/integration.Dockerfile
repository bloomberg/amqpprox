FROM rabbitmq:4.3.0

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for integration tests
RUN apt-get update && apt-get dist-upgrade -y && \
    apt-get install -y python3 python3-pip python3-venv \
    curl llvm make cmake build-essential npm
RUN python3 -m pip install --break-system-packages setuptools "conan>=2,<3" robotframework pika amqp pytest
ENV HOME="/root" PATH="/root/.cargo/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y

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
RUN npm install
WORKDIR /source

RUN make setup && make init && make
ENV ROBOT_SOURCE_DIR=/source/tests/acceptance ROBOT_BINARY_DIR=/opt/rabbitmq/sbin
ENV AMQPPROX_BIN_DIR=/build/bin
ENV NODE_OPTIONS=--dns-result-order=ipv4first
