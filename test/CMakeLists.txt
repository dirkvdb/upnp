INCLUDE_DIRECTORIES(
    ${UTILS_INCLUDE_DIRS}
    ${UPNPFRAMEWORK_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

INCLUDE_DIRECTORIES(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/testframework
)

LINK_DIRECTORIES(
    ${UPNPFRAMEWORK_LIBRARY_DIRS}
)

#     devicediscover.h
#     controlpointtest.cpp
#     testenvironment.h

ADD_EXECUTABLE(upnpframeworktest
    testframework/gmock-gtest-all.cpp
    main.cpp

    testenvironment.h
    upnp.clientmock.h
    upnp.rootdevicemock.h

    testxmls.h
    upnp.utilstest.cpp
    upnp.clienttest.cpp
    upnp.rootdevicetest.cpp
    upnp.ssdp.clienttest.cpp
    upnp.ssdp.servertest.cpp
    upnp.ssdp.test.cpp
    upnp.http.clienttest.cpp
    upnp.http.parsertest.cpp
    upnp.http.readertest.cpp
    upnp.gena.servertest.cpp
    upnp.actiontest.cpp
    upnp.devicescannertest.cpp
    upnp.xml.parseutilstest.cpp
    upnp.soap.parseutilstest.cpp

    upnp.clienttestbase.h
    upnp.avtransport.clienttest.cpp
    upnp.renderingcontrol.clienttest.cpp
    upnp.connectionmanager.clienttest.cpp
    upnp.contentdirectory.clienttest.cpp

    upnp.avtransport.servicetest.cpp
    upnp.connectionmanager.servicetest.cpp
    upnp.renderingcontrol.servicetest.cpp

    upnp.mediaservertest.cpp
    upnp.mediarenderertest.cpp
)

FIND_PATH(BENCHMARK_INCLUDE_DIR
    NAMES benchmark/benchmark.h
)

FIND_LIBRARY(BENCHMARK_LIBRARY
    NAMES benchmark
)

IF (BENCHMARK_INCLUDE_DIR AND BENCHMARK_LIBRARY)
    ADD_EXECUTABLE(upnpbenchmark
        upnp.xml.parseutilsbenchmark.cpp
        benchmark.cpp
    )

    TARGET_LINK_LIBRARIES(upnpbenchmark upnpframework ${BENCHMARK_LIBRARY})

    IF (MINGW OR WIN32)
        TARGET_LINK_LIBRARIES(upnpbenchmark upnpframework Shlwapi)
    ENDIF ()
ENDIF ()

TARGET_LINK_LIBRARIES(upnpframeworktest
    upnpframework
)

IF (MINGW)
    TARGET_LINK_LIBRARIES(upnpframeworktest
        -lIphlpapi -lws2_32 -lwsock32
    )
ENDIF()

ADD_TEST(NAME UPnPFrameworkTests COMMAND upnpframeworktest)
