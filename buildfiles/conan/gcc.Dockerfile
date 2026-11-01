FROM ubuntu:26.04@sha256:6a06111c3b0a8600feadb270aec771dc239bf407b25625f93f44ab00e11f4d3c AS amqpprox_build_environment

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
