cmake_minimum_required(VERSION 3.1)
enable_testing()

set(CMAKE_MODULES_PATH modules)

find_package(PkgConfig)

if (STD_ASIO)
    find_path(ASIO_INCLUDE_DIR
        NAMES asio.hpp
    )

    if (NOT ASIO_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find the asio headers")
    endif ()

    add_definitions("-DASIO_STANDALONE")
else ()
    set (Boost_USE_STATIC_LIBS ON)
    add_definitions("-DBOOST_ALL_NO_LIB")
    find_package(Boost REQUIRED COMPONENTS system)
endif ()

find_path(BEAST_INCLUDE_DIR
    NAMES beast/http.hpp
)

if (NOT BEAST_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find the beast http headers")
endif ()

if (CMAKE_GENERATOR STREQUAL Xcode)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
elseif (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
endif ()

option(STANDALONE "Not used as a submodule of another project" OFF)
option(ENABLE_TESTS "build unit tests" ON)

if (STANDALONE)
    add_subdirectory(../utils utils)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1")
    set(CMAKE_CXX_STANDARD 14)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif ()
endif ()

if (CMAKE_GENERATOR STREQUAL Xcode)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
endif ()

add_library(upnpframework STATIC
    inc/upnp/upnp.serviceclientbase.h
    inc/upnp/upnp.avtransport.types.h
    inc/upnp/upnp.avtransport.client.h                  src/upnp.avtransport.client.cpp
    inc/upnp/upnp.avtransport.service.h                 src/upnp.avtransport.service.cpp
    inc/upnp/upnp.clientinterface.h
    inc/upnp/upnp.connectionmanager.client.h            src/upnp.connectionmanager.client.cpp
    inc/upnp/upnp.connectionmanager.service.h           src/upnp.connectionmanager.service.cpp
    inc/upnp/upnp.connectionmanager.types.h
    inc/upnp/upnp.contentdirectory.client.h             src/upnp.contentdirectory.client.cpp
    inc/upnp/upnp.contentdirectory.service.h            src/upnp.contentdirectory.service.cpp
    inc/upnp/upnp.contentdirectory.types.h
    inc/upnp/upnp.controlpoint.h                        src/upnp.controlpoint.cpp
    inc/upnp/upnp.device.h
    inc/upnp/upnp.deviceservice.h
    inc/upnp/upnp.devicescanner.h                       src/upnp.devicescanner.cpp
    inc/upnp/upnp.dlna.info.h                           src/upnp.dlna.info.cpp
    inc/upnp/upnp.factory.h                             src/upnp.factory.cpp
    inc/upnp/upnpfwd.h
    inc/upnp/upnp.item.h                                src/upnp.item.cpp
    inc/upnp/upnp.lastchangevariable.h                  src/upnp.lastchangevariable.cpp
    inc/upnp/upnp.mediarenderer.h                       src/upnp.mediarenderer.cpp
    inc/upnp/upnp.mediaserver.h                         src/upnp.mediaserver.cpp
    inc/upnp/upnp.protocolinfo.h                        src/upnp.protocolinfo.cpp
    inc/upnp/upnp.renderingcontrol.types.h
    inc/upnp/upnp.renderingcontrol.client.h             src/upnp.renderingcontrol.client.cpp
    inc/upnp/upnp.rootdevice.h                          src/upnp.rootdevice.cpp
    inc/upnp/upnp.renderingcontrol.service.h            src/upnp.renderingcontrol.service.cpp
    inc/upnp/upnp.rootdeviceinterface.h
    inc/upnp/upnpservicevariable.h
    inc/upnp/upnpstatevariable.h
    inc/upnp/upnptypes.h
    inc/upnp/upnp.types.h                               src/upnp.types.cpp
    inc/upnp/upnp.flags.h
    inc/upnp/upnputils.h
    inc/upnp/upnp.action.h                              src/upnp.action.cpp
    inc/upnp/upnp.actionresponse.h                      src/upnp.actionresponse.cpp
    inc/upnp/upnp.soap.types.h
    inc/upnp/upnp.ssdp.client.h                         src/upnp.ssdp.client.cpp
    inc/upnp/upnp.ssdp.server.h                         src/upnp.ssdp.server.cpp
    inc/upnp/upnp.http.functions.h                      src/upnp.http.functions.cpp
    inc/upnp/upnp.http.functions.h
    inc/upnp/upnp.http.types.h                          src/upnp.http.types.cpp
    inc/upnp/upnp.http.reader.h                         src/upnp.http.reader.cpp
    inc/upnp/upnp.http.server.h                         src/upnp.http.server.cpp
    inc/upnp/upnp.http.parser.h                         src/upnp.http.parser.cpp
    inc/upnp/upnp.http.parseutils.h
    inc/upnp/upnp.xml.parseutils.h                      src/upnp.xml.parseutils.cpp
    inc/upnp/upnp.servicefaults.h
    inc/upnp/stringview.h

    src/upnp.enumutils.h
    src/upnp.connectionmanager.typeconversions.h        src/upnp.connectionmanager.typeconversions.cpp
    src/upnp.contentdirectory.typeconversions.h         src/upnp.contentdirectory.typeconversions.cpp
    src/upnp.renderingcontrol.typeconversions.h         src/upnp.renderingcontrol.typeconversions.cpp
    src/upnp.avtransport.typeconversions.h              src/upnp.avtransport.typeconversions.cpp
    src/upnp.client.h                                   src/upnp.client.cpp
    src/upnp.gena.server.h                              src/upnp.gena.server.cpp
    src/upnp.http.utils.h                               src/upnp.http.utils.cpp
    src/upnp.http.client.h                              src/upnp.http.client.cpp
    src/upnp.soap.client.h                              src/upnp.soap.client.cpp
    src/upnp.ssdp.parseutils.h
    src/http_parser.h                                   src/http_parser.cpp
    src/URI.h                                           src/URI.cpp
    src/guid.h                                          src/guid.cpp
)

target_include_directories(upnpframework PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
    ${Boost_INCLUDE_DIRS}
    PRIVATE ${CMAKE_BINARY_DIR}
)

target_include_directories(upnpframework SYSTEM PRIVATE
    ${BEAST_INCLUDE_DIR}
)

target_link_libraries(upnpframework utils)

if (MINGW)
    target_link_libraries(upnpframework Iphlpapi ws2_32 Mswsock)
endif ()

if (APPLE)
    # uuid dependency
    target_link_libraries(upnpframework "-framework CoreFoundation")
ELSEif (UNIX)
    # uuid dependency
    target_link_libraries(upnpframework "uuid")
endif ()

if (NOT STD_ASIO)
    target_link_libraries(upnpframework ${Boost_LIBRARIES})
endif ()

if (ENABLE_TESTS)
    add_subdirectory(test)
endif ()
