FROM ubuntu:24.04 AS amqpprox_build_environment

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

RUN python3 -m pip install --break-system-packages "conan>=2,<3"

WORKDIR /source
ENV BUILDDIR=/build
ENV CONAN_HOME=/build/.conan2
