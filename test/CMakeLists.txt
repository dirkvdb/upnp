include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/testframework
)

add_executable(upnpframeworktest
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
    upnp.soap.clienttest.cpp
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

find_path(BENCHMARK_INCLUDE_DIR
    NAMES benchmark/benchmark.h
)

find_library(BENCHMARK_LIBRARY
    NAMES benchmark
)

if (BENCHMARK_INCLUDE_DIR AND BENCHMARK_LIBRARY)
    add_executable(upnpbenchmark
        upnp.xml.parseutilsbenchmark.cpp
        benchmark.cpp
    )

    target_link_libraries(upnpbenchmark upnpframework ${BENCHMARK_LIBRARY})

    if (MINGW OR WIN32)
        target_link_libraries(upnpbenchmark Shlwapi)
    endif ()
endif ()

target_link_libraries(upnpframeworktest
    upnpframework
)

if (MINGW)
    target_link_libraries(upnpframeworktest
        Iphlpapi ws2_32 wsock32
    )
endif()

add_test(NAME UPnPFrameworkTests COMMAND upnpframeworktest)
