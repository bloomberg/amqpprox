FROM ubuntu:26.04@sha256:d66b366dac61b575bee2aef07e9c26e8070096fbe84a3d4d3eb0da069b5d76db AS amqpprox_build_environment

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    ninja-build \
    build-essential \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    llvm \
    clang \
    && rm -rf /var/lib/apt/lists/*

COPY buildfiles/conan/requirements.txt /tmp/requirements.txt
RUN python3 -m pip install --break-system-packages --require-hashes -r /tmp/requirements.txt

WORKDIR /source
ENV BUILDDIR=/build
ENV CONAN_HOME=/build/.conan2
