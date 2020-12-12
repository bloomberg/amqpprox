include("${BUILD_FLAVOUR_DIR}/amqpprox_ctl.pre.cmake" OPTIONAL)

add_executable(amqpprox_ctl client.m.cpp)
target_link_libraries(amqpprox_ctl LINK_PUBLIC libamqpprox ${AMQPPROX_CTL_LIBS})

install(TARGETS amqpprox_ctl DESTINATION bin COMPONENT amqpprox)

include("${BUILD_FLAVOUR_DIR}/amqpprox_ctl.post.cmake" OPTIONAL)
