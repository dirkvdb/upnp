CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
ENABLE_TESTING()

SET(CMAKE_MODULES_PATH modules)

FIND_PACKAGE(PkgConfig)
FIND_PATH(ASIO_INCLUDE_DIR
    NAMES asio.hpp
)

IF (NOT ASIO_INCLUDE_DIR)
    MESSAGE(FATAL_ERROR "Could not find the asio headers")
ENDIF ()

ADD_DEFINITIONS("-DASIO_STANDALONE")

OPTION(STANDALONE "Not used as a submodule of another project" OFF)
OPTION(ENABLE_TESTS "build unit tests" ON)

IF (STANDALONE)
    ADD_SUBDIRECTORY(../utils utils)
    GET_DIRECTORY_PROPERTY(UTILS_INCLUDE_DIRS   DIRECTORY ../utils INCLUDE_DIRECTORIES)

    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1")
    ADD_DEFINITIONS("-std=c++14")
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND APPLE)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    ENDIF ()
ENDIF ()

IF (CMAKE_GENERATOR STREQUAL Xcode)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
ENDIF ()

SET (UPNPFRAMEWORK_SYS_INCLUDE_DIRS
    ${ASIO_INCLUDE_DIR}
)

SET (UPNPFRAMEWORK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
    ${UTILS_INCLUDE_DIRS}
)

SET (UPNPFRAMEWORK_LIBRARY_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}"
)

INCLUDE_DIRECTORIES(${UPNPFRAMEWORK_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(SYSTEM
    ${UPNPFRAMEWORK_SYS_INCLUDE_DIRS}
)

ADD_LIBRARY(upnpframework STATIC
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
    inc/upnp/upnpdeviceserviceexceptions.h
    inc/upnp/upnp.devicescanner.h                       src/upnp.devicescanner.cpp
    inc/upnp/upnp.dlna.info.h                           src/upnp.dlna.info.cpp
    inc/upnp/upnp.factory.h                             src/upnp.factory.cpp
    inc/upnp/upnpfwd.h
    inc/upnp/upnp.http.reader.h                         src/upnp.http.reader.cpp
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
    inc/upnp/upnp.ssdp.client.h                         src/upnp.ssdp.client.cpp
    inc/upnp/upnp.ssdp.server.h                         src/upnp.ssdp.server.cpp
    inc/upnp/upnp.http.client.h                         src/upnp.http.client.cpp
    inc/upnp/upnp.http.server.h                         src/upnp.http.server.cpp
    inc/upnp/upnp.http.parser.h                         src/upnp.http.parser.cpp
    inc/upnp/upnp.xml.parseutils.h                      src/upnp.xml.parseutils.cpp

    src/upnp.enumutils.h
    src/upnp.connectionmanager.typeconversions.h        src/upnp.connectionmanager.typeconversions.cpp
    src/upnp.contentdirectory.typeconversions.h         src/upnp.contentdirectory.typeconversions.cpp
    src/upnp.renderingcontrol.typeconversions.h         src/upnp.renderingcontrol.typeconversions.cpp
    src/upnp.avtransport.typeconversions.h              src/upnp.avtransport.typeconversions.cpp
    src/upnp.client.h                                   src/upnp.client.cpp
    src/upnp.gena.server.h                              src/upnp.gena.server.cpp
    src/upnp.ssdp.parseutils.h
    src/stringview.h
    src/http_parser.h                                   src/http_parser.cpp
    src/URI.h                                           src/URI.cpp
    src/guid.h                                          src/guid.cpp
)

TARGET_LINK_LIBRARIES(upnpframework utils)

IF (MINGW)
    TARGET_LINK_LIBRARIES(upnpframework Iphlpapi ws2_32 Mswsock)
ENDIF ()

IF (APPLE)
    # uuid dependency
    TARGET_LINK_LIBRARIES(upnpframework "-framework CoreFoundation")
ELSEIF (UNIX)
    # uuid dependency
    TARGET_LINK_LIBRARIES(upnpframework "uuid")
ENDIF ()

IF (ENABLE_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF ()
