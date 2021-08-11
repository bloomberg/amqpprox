FROM rabbitmq:3.7.9

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get dist-upgrade -y --force-yes
RUN apt-get install -y --force-yes python3.8 python3.8-distutils \
    curl llvm make cmake build-essential npm
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3.8 get-pip.py

RUN python3.8 -m pip install setuptools conan robotframework pika amqp

EXPOSE 15800
EXPOSE 15801

ENV BUILDDIR=/build
ENV CONAN_USER_HOME=/build

COPY . /source
WORKDIR /source/tests/acceptance/integration
RUN npm install
WORKDIR /source

RUN make setup && make init && make
ENV ROBOT_SOURCE_DIR=/source/tests/acceptance
ENV ROBOT_BINARY_DIR=/opt/rabbitmq/sbin
ENTRYPOINT [ "/source/tests/acceptance/run.sh"]
