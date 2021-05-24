# buildfiles: Conan Edition

This provides a set of buildfiles using the Conan system and Conan Docker base
images in order to build the dependencies of amqpprox. The primary dependencies
are boost, gtest and OpenSSL. These are listed in the `conanfile.txt`.

If the arguments to `conan install` need to be changed, a `OVERRIDE_CONANOPTS`
environment variable can be passed during the `make setup` bootstrap phase.
This is likely to be needed if building with different toolchains/profiles.

We default the Docker based build to a clang base image, but this is
configurable through the `BUILD_DOCKERFILE` environment variable.
