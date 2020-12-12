include("${BUILD_FLAVOUR_DIR}/amqpprox.pre.cmake" OPTIONAL)

add_executable(amqpprox amqpprox.m.cpp)
target_link_libraries(amqpprox LINK_PUBLIC libamqpprox ${AMQPPROX_LIBS})

install(TARGETS amqpprox DESTINATION bin COMPONENT amqpprox)

include("${BUILD_FLAVOUR_DIR}/amqpprox.post.cmake" OPTIONAL)
