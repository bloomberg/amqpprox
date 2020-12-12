FROM conanio/gcc9 AS amqpprox_build_environment
WORKDIR /source
ENV BUILDDIR=/build
ENV CONAN_USER_HOME=/build
