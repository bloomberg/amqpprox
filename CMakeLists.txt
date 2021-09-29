cmake_minimum_required(VERSION 3.9)

project(amqpprox)

if(DEFINED ENV{BUILD_FLAVOUR_OVERRIDE})
    set(BUILD_FLAVOUR $ENV{BUILD_FLAVOUR_OVERRIDE})
else(DEFINED ENV{BUILD_FLAVOUR_OVERRIDE})
    file(READ "${CMAKE_BINARY_DIR}/flavour" BUILD_FLAVOUR)
endif(DEFINED ENV{BUILD_FLAVOUR_OVERRIDE})

set(BUILD_FLAVOUR_DIR "${CMAKE_SOURCE_DIR}/buildfiles/${BUILD_FLAVOUR}")

set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

enable_testing()

include("${BUILD_FLAVOUR_DIR}/main.pre.cmake")

# Find threads
find_package(Threads)

# Proto files to serialize/deserialize auth request/response data
add_subdirectory(authproto)

# Main library
add_subdirectory(libamqpprox)

# Unit tests
add_subdirectory(tests)

# Server executable
add_subdirectory(amqpprox)

# Control executable
add_subdirectory(amqpprox_ctl)

include("${BUILD_FLAVOUR_DIR}/main.post.cmake" OPTIONAL)

