FROM conanio/clang9 AS amqpprox_build_environment
RUN sudo apt-get update && sudo apt-get install -y llvm
WORKDIR /source
ENV BUILDDIR=/build
ENV CONAN_USER_HOME=/build
