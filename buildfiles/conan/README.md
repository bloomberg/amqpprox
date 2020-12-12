# buildfiles: Conan Edition

This provides a set of buildfiles using the Conan system and Conan Docker base
images in order to build the dependencies of amqpprox. The primary dependencies
are boost, gtest and OpenSSL. These are listed in the `conanfile.txt`.

We default the Docker based build to a clang base image, but this is
configurable through the `BUILD_DOCKERFILE` environment variable.

