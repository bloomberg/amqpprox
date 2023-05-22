FROM conanio/clang9 AS builder
RUN sudo apt-get update && sudo apt-get install -y llvm
ENV BUILDDIR=/home/conan/build
ENV CONAN_USER_HOME=/home/conan
COPY . ./source
WORKDIR /home/conan/source
RUN make setup && make init && make

FROM ubuntu:focal
WORKDIR /amqpproxy
COPY --from=builder /home/conan/build/bin/ ./
RUN useradd -ms /bin/bash amqpproxy && \
    chown -R amqpproxy /amqpproxy && \
    chmod -R 755 /amqpproxy
USER amqpproxy
CMD ["./amqpprox"]
