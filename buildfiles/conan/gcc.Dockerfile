FROM ubuntu:24.04@sha256:24737cef17f4e22b1977df7d9a05e77c4e28b7da5b8911f8d8ccda136bf3573c AS amqpprox_build_environment

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
