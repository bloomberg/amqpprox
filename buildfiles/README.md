# buildfiles

This directory provides a pluggable cmake-based build system for building
amqpprox. This is to allow internal and specific to Bloomberg build systems to
be overlayed without affecting the repository structure or settings with
conditional options. It also keeps common parameters, such as the files
included in the shared part of the project, so they do not readily get out of
sync.

## Requirements for buildfile implementations

Each build system needs to present itself through a set of common
hooks as listed below:

1. `bootstrap`: An executable script called at set up time, it is not required
   to be present.
2. `Dockerfile`: A Dockerfile for bringing up the buildsystem without any
   specific dependencies on the build machine. No ENTRYPOINT is required, but
   the Dockerfile must exist to allow docker based builds.
3. `*.pre.cmake` and `*.post.cmake`: Hooks into the beginning and end of the
   `CMakeLists.txt` file for each of the directories. These files are optional
   and no error will be produced if one of the combinations is not required.

## User Customisation

The makefile part of the system can be customised with a number of environment
variables.

Overall:

- `BUILD_FLAVOUR`: Used at setup time to select which set of buildfiles to be
    used. Selects a sub-directory of the buildfiles directory.
- `CUR_DIR`: The directory of the makefile (defaults to `pwd`)

For native builds:

- `UNAME_OVERRIDE`: Override the uname of the system.
- `BUILDDIR`: Override the build directory to be used for native builds, by
    default it will be `build/$(UNAME_OVERRIDE)`.
- `EXTRA_CMAKE_ARGS`: More arguments to be passed to the cmake invocation, for
    ie changing build type.
- `BUILD_PARALLELISM`: The number of jobs to use for builds, defaults to the
    number of cores.

For docker builds:

- `DOCKER_IMAGE`: Name of the docker image to build and run commands against.
    This allows multiple docker builds to be run from the same directory with
    different configurations. This defaults to 'amqpprox'.
- `DOCKER_BUILDDIR`: Override the build directory for the docker-based
    builds, by default it will be `build/docker-$(DOCKER_IMAGE)`.
- `DOCKER_EXTRA_ARGS`: Provide extra arguments to any docker invocation
- `BUILD_DOCKERFILE`: Provides a specific dockerfile to be used in the setup
    and build.
- `DOCKER_ARGS` and `DOCKER_RUN`: These are both overrideable, but not designed
    to be used for normal customisation.
