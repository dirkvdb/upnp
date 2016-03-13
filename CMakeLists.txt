CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
ENABLE_TESTING()

SET(CMAKE_MODULES_PATH modules)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP_PKG libupnp REQUIRED)

FIND_PATH(UPNP_INCLUDE_DIR
    NAMES upnp.h
    PATHS ${UPNP_PKG_INCLUDE_DIRS}
)

FIND_LIBRARY(UPNP_LIBRARY
    NAMES upnp
    PATHS ${UPNP_PKG_LIBRARY_DIRS}
)

FIND_LIBRARY(UPNP_LIBRARY
    NAMES threadutil
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

OPTION(STANDALONE "Not used as a submodule of another project" OFF)
OPTION(ENABLE_TESTS "build unit tests" ON)

IF (STANDALONE)
    ADD_SUBDIRECTORY(../utils utils)
    GET_DIRECTORY_PROPERTY(UTILS_INCLUDE_DIRS   DIRECTORY ../utils INCLUDE_DIRECTORIES)

    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1")
    ADD_DEFINITIONS("-std=c++1y")
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND APPLE)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    ENDIF ()
ENDIF ()

SET (UPNPFRAMEWORK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
    ${UTILS_INCLUDE_DIRS}
    ${UPNP_INCLUDE_DIR}
)
SET (UPNPFRAMEWORK_LIBRARY_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}"
)

INCLUDE_DIRECTORIES(${UPNPFRAMEWORK_INCLUDE_DIRS})

ADD_LIBRARY(upnpframework STATIC
    inc/upnp/upnpaction.h                       src/upnpaction.cpp
    inc/upnp/upnpactionresponse.h               src/upnpactionresponse.cpp
    inc/upnp/upnpavtransportclient.h            src/upnpavtransportclient.cpp
    inc/upnp/upnpavtransportservice.h           src/upnpavtransportservice.cpp
    inc/upnp/upnpavtransporttypes.h
    inc/upnp/upnpclientinterface.h
    inc/upnp/upnpconnectionmanagerclient.h      src/upnpconnectionmanagerclient.cpp
    inc/upnp/upnpconnectionmanagerservice.h     src/upnpconnectionmanagerservice.cpp
    inc/upnp/upnpconnectionmanagertypes.h
    inc/upnp/upnpcontentdirectoryclient.h       src/upnpcontentdirectoryclient.cpp
    inc/upnp/upnpcontentdirectoryservice.h      src/upnpcontentdirectoryservice.cpp
    inc/upnp/upnpcontentdirectorytypes.h
    inc/upnp/upnpcontrolpoint.h                 src/upnpcontrolpoint.cpp
    inc/upnp/upnpdevice.h
    inc/upnp/upnpdeviceservice.h
    inc/upnp/upnpdeviceserviceexceptions.h
    inc/upnp/upnpdevicescanner.h                src/upnpdevicescanner.cpp
    inc/upnp/upnpdlnainfo.h                     src/upnpdlnainfo.cpp
    inc/upnp/upnpfactory.h                      src/upnpfactory.cpp
    inc/upnp/upnpfwd.h
    inc/upnp/upnphttpclient.h                   src/upnphttpclient.cpp
    inc/upnp/upnphttpreader.h                   src/upnphttpreader.cpp
    inc/upnp/upnpitem.h                         src/upnpitem.cpp
    inc/upnp/upnplastchangevariable.h           src/upnplastchangevariable.cpp
    inc/upnp/upnpmediarenderer.h                src/upnpmediarenderer.cpp
    inc/upnp/upnpmediaserver.h                  src/upnpmediaserver.cpp
    inc/upnp/upnpprotocolinfo.h                 src/upnpprotocolinfo.cpp
    inc/upnp/upnprenderingcontrolclient.h       src/upnprenderingcontrolclient.cpp
    inc/upnp/upnprenderingcontrolservice.h      src/upnprenderingcontrolservice.cpp
    inc/upnp/upnprenderingcontroltypes.h
    inc/upnp/upnprootdevice.h                   src/upnprootdevice.cpp
    inc/upnp/upnprootdeviceinterface.h
    inc/upnp/upnpserviceclientbase.h
    inc/upnp/upnpservicevariable.h
    inc/upnp/upnpstatevariable.h
    inc/upnp/upnptypes.h
    inc/upnp/upnputils.h
    inc/upnp/upnpwebserver.h                    src/upnpwebserver.cpp
    inc/upnp/upnpxml.h                          src/upnpxml.cpp
    inc/upnp/upnpxmlutils.h                     src/upnpxmlutils.cpp
    src/upnpclient.h                            src/upnpclient.cpp
)

TARGET_LINK_LIBRARIES(upnpframework ${UPNP_LIBRARY} ${THREADUTIL_LIBRARY} ${IXML_LIBRARY} utils)

IF (ENABLE_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF ()
