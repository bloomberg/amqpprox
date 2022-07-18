include("${BUILD_FLAVOUR_DIR}/tests.pre.cmake" OPTIONAL)

include_directories(.)

add_library(amqpprox_testapparatus
    amqpprox_socketintercepttestadaptor.cpp
    amqpprox_testsocketstate.cpp)

target_link_libraries(amqpprox_testapparatus
    libamqpprox)

add_executable(amqpprox_tests
    amqpprox_affinitypartitionpolicy.t.cpp
    amqpprox_backend.t.cpp
    amqpprox_backendstore.t.cpp
    amqpprox_backendselectorstore.t.cpp
    amqpprox_buffer.t.cpp
    amqpprox_bufferhandle.t.cpp
    amqpprox_bufferpool.t.cpp
    amqpprox_buffersource.t.cpp
    amqpprox_connectionstats.t.cpp
    amqpprox_dataratelimit.t.cpp
    amqpprox_dnsresolver.t.cpp
    amqpprox_eventsourcesignal.t.cpp
    amqpprox_farmstore.t.cpp
    amqpprox_frame.t.cpp
    amqpprox_flowtype.t.cpp
    amqpprox_connectionselector.t.cpp
    amqpprox_methods_start.t.cpp
    amqpprox_packetprocessor.t.cpp
    amqpprox_partitionpolicystore.t.cpp
    amqpprox_proxyprotocolheaderv1.t.cpp
    amqpprox_resourcemapper.t.cpp
    amqpprox_robinbackendselector.t.cpp
    amqpprox_session.t.cpp
    amqpprox_sessionstate.t.cpp
    amqpprox_statcollector.t.cpp
    amqpprox_statsnapshot.t.cpp
    amqpprox_types.t.cpp
    amqpprox_vhoststate.t.cpp
    amqpprox_defaultauthintercept.t.cpp
    amqpprox_httpauthintercept.t.cpp
    amqpprox_fixedwindowconnectionratelimiter.t.cpp
    amqpprox_totalconnectionlimiter.t.cpp
    amqpprox_connectionlimitermanager.t.cpp
    )

target_include_directories(amqpprox_tests PRIVATE ${PROTO_HDR_PATH})
target_link_libraries(amqpprox_tests LINK_PUBLIC
    authproto
    libamqpprox
    amqpprox_testapparatus
    gtest
    gmock
    gtest_main
    gmock_main
    ${AMQPPROX_TEST_LIBS})

add_test(NAME Tests COMMAND $<TARGET_FILE:amqpprox_tests>)

include("${BUILD_FLAVOUR_DIR}/tests.post.cmake" OPTIONAL)
