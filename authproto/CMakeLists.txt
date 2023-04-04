set(Protobuf_USE_STATIC_LIBS ON)

# XXX(acasey23): Can we revisit this when we get to conan 2? This feels like a bug in the protobuf conan package
set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} ${CONAN_BIN_DIRS})
include(FindProtobuf)
find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS sasl.proto authrequest.proto authresponse.proto)
add_library(authproto ${PROTO_SRCS} ${PROTO_HDRS})
set(PROTO_HDR_PATH ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
target_link_libraries(authproto PUBLIC ${Protobuf_LIBRARIES})
