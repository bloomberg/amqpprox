set(Protobuf_USE_STATIC_LIBS ON)

find_package(protobuf REQUIRED CONFIG)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS sasl.proto authrequest.proto authresponse.proto)
add_library(authproto ${PROTO_SRCS} ${PROTO_HDRS})
set(PROTO_HDR_PATH ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
target_link_libraries(authproto PUBLIC protobuf::libprotobuf)
