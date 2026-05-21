message(STATUS "** Using conan build")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 17)
endif()

if (NOT "${SANITIZER}" STREQUAL "" )
    message(STATUS "** Using sanitizer: ${SANITIZER}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${SANITIZER}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${SANITIZER}")
endif()

if (NOT "${COVERAGE}" STREQUAL "")
    message(STATUS "** Enabling code coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

find_package(Boost REQUIRED COMPONENTS log log_setup system thread timer program_options)
find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)
find_package(protobuf REQUIRED)
