CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
ENABLE_TESTING()

SET(CMAKE_MODULES_PATH modules)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP_PKG libupnp REQUIRED)
PKG_CHECK_MODULES(LIBUV_PKG libuv REQUIRED)
PKG_CHECK_MODULES(CURL_PKG libcurl REQUIRED)

FIND_PATH(UPNP_INCLUDE_DIR
    NAMES upnp.h
    PATHS ${UPNP_PKG_INCLUDE_DIRS}
)

FIND_LIBRARY(UPNP_LIBRARY
    NAMES upnp
    PATHS ${UPNP_PKG_LIBRARY_DIRS}
)

FIND_LIBRARY(IXML_LIBRARY
    NAMES ixml
    PATHS ${UPNP_PKG_LIBRARY_DIRS}
)

FIND_LIBRARY(THREADUTIL_LIBRARY
    NAMES threadutil
    PATHS ${UPNP_PKG_LIBRARY_DIRS}
)

FIND_PATH(LIBUV_INCLUDE_DIR
    NAMES uv.h
    PATHS ${LIBUV_PKG_INCLUDE_DIRS}
)

FIND_LIBRARY(LIBUV_LIBRARY
    NAMES uv
    PATHS ${LIBUV_PKG_LIBRARY_DIRS}
)

FIND_PATH(CURL_INCLUDE_DIR
    NAMES curl/curl.h
    PATHS ${CURL_PKG_INCLUDE_DIRS}
)

FIND_LIBRARY(CURL_LIBRARY
    NAMES curl
    PATHS ${CURL_PKG_LIBRARY_DIRS}
)

FIND_PATH(MONGOOSE_INCLUDE_DIR
    NAMES mongoose.h
)

FIND_LIBRARY(MONGOOSE_LIBRARY
    NAMES mongoose
)

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

SET (UPNPFRAMEWORK_SYS_INCLUDE_DIRS
    ${LIBUV_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${MONGOOSE_INCLUDE_DIR}
)

SET (UPNPFRAMEWORK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
    ${UTILS_INCLUDE_DIRS}
    ${UPNP_INCLUDE_DIR}
)

SET (UPNPFRAMEWORK_LIBRARY_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}"
)

INCLUDE_DIRECTORIES(${UPNPFRAMEWORK_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gsl
    ${UPNPFRAMEWORK_SYS_INCLUDE_DIRS}
)

ADD_LIBRARY(upnpframework STATIC
    inc/upnp/upnpactionresponse.h               src/upnpactionresponse.cpp
    inc/upnp/upnp.avtransport.types.h
    inc/upnp/upnp.avtransport.client.h          src/upnp.avtransport.client.cpp
    inc/upnp/upnpavtransportservice.h           src/upnpavtransportservice.cpp
    inc/upnp/upnpclientinterface.h
    inc/upnp/upnp.clientinterface.h
    inc/upnp/upnp.connectionmanager.client.h    src/upnp.connectionmanager.client.cpp
    inc/upnp/upnpconnectionmanagerservice.h     src/upnpconnectionmanagerservice.cpp
    inc/upnp/upnp.connectionmanager.types.h
    inc/upnp/upnp.contentdirectory.client.h     src/upnp.contentdirectory.client.cpp
    inc/upnp/upnpcontentdirectoryservice.h      src/upnpcontentdirectoryservice.cpp
    inc/upnp/upnp.contentdirectory.types.h
    inc/upnp/upnpcontrolpoint.h                 src/upnpcontrolpoint.cpp
    inc/upnp/upnpdevice.h
    inc/upnp/upnpdeviceservice.h
    inc/upnp/upnpdeviceserviceexceptions.h
    inc/upnp/upnp.devicescanner.h               src/upnp.devicescanner.cpp
    inc/upnp/upnpdlnainfo.h                     src/upnpdlnainfo.cpp
    inc/upnp/upnpfactory.h                      src/upnpfactory.cpp
    inc/upnp/upnp.factory.h                     src/upnp.factory.cpp
    inc/upnp/upnpfwd.h
    inc/upnp/upnphttpclient.h                   src/upnphttpclient.cpp
    inc/upnp/upnphttpreader.h                   src/upnphttpreader.cpp
    inc/upnp/upnpitem.h                         src/upnpitem.cpp
    inc/upnp/upnplastchangevariable.h           src/upnplastchangevariable.cpp
    inc/upnp/upnpmediarenderer.h                src/upnpmediarenderer.cpp
    inc/upnp/upnpmediaserver.h                  src/upnpmediaserver.cpp
    inc/upnp/upnpprotocolinfo.h                 src/upnpprotocolinfo.cpp
    inc/upnp/upnp.renderingcontrol.types.h
    inc/upnp/upnp.renderingcontrol.client.h     src/upnp.renderingcontrol.client.cpp
    inc/upnp/upnprenderingcontrolservice.h      src/upnprenderingcontrolservice.cpp
    inc/upnp/upnprootdevice.h                   src/upnprootdevice.cpp
    inc/upnp/upnprootdeviceinterface.h
    inc/upnp/upnpserviceclientbase.h
    inc/upnp/upnpservicevariable.h
    inc/upnp/upnpstatevariable.h
    inc/upnp/upnptypes.h
    inc/upnp/upnp.types.h
    inc/upnp/upnp.flags.h
    inc/upnp/upnputils.h
    inc/upnp/upnpwebserver.h                    src/upnpwebserver.cpp
    inc/upnp/upnpxml.h                          src/upnpxml.cpp
    inc/upnp/upnpxmlutils.h                     src/upnpxmlutils.cpp
    inc/upnp/upnp.action.h                      src/upnp.action.cpp
    inc/upnp/upnp.ssdp.client.h                 src/upnp.ssdp.client.cpp
    inc/upnp/upnp.http.client.h                 src/upnp.http.client.cpp
    inc/upnp/upnp.http.server.h                 src/upnp.http.server.cpp
    inc/upnp/upnp.xml.parseutils.h              src/upnp.xml.parseutils.cpp
    inc/upnp/upnp.uv.h
    src/upnp.client.h                           src/upnp.client.cpp
    src/upnp.gena.server.h                      src/upnp.gena.server.cpp
    src/upnpclient.h                            src/upnpclient.cpp
    src/upnp.http.parser.h
    src/upnp.ssdp.parseutils.h
    src/http_parser.h                           src/http_parser.cpp
    src/URI.h                                   src/URI.cpp
)

TARGET_LINK_LIBRARIES(upnpframework utils ${LIBUV_LIBRARY} ${CURL_LIBRARY} ${MONGOOSE_LIBRARY} ${UPNP_LIBRARY} ${IXML_LIBRARY} ${THREADUTIL_LIBRARY})

IF (MINGW)
    TARGET_LINK_LIBRARIES(upnpframework ws2_32)
ENDIF ()

IF (ENABLE_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF ()
