FROM ubuntu:25.10@sha256:7cc5e35f6567ee8c66d2abb4aab0fd866669e6207c237c3a8f0947a5c7f17092 AS amqpprox_build_environment

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
    g++ \
    && rm -rf /var/lib/apt/lists/*

COPY buildfiles/conan/requirements.txt /tmp/requirements.txt
RUN python3 -m pip install --break-system-packages --require-hashes -r /tmp/requirements.txt

WORKDIR /source
ENV BUILDDIR=/build
ENV CONAN_HOME=/build/.conan2
