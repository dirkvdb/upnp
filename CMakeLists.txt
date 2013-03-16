CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ENABLE_TESTING()

SET(CMAKE_MODULES_PATH modules)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP libupnp REQUIRED)

SET (UPNPFRAMEWORK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../utils/inc/"
    ${UPNP_INCLUDE_DIRS})
SET (UPNPFRAMEWORK_LIBRARY_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}"
    ${UPNP_LIBRARY_DIRS}
)
SET (UPNPFRAMEWORK_LIBRARIES upnpframework ${UPNP_LIBRARIES} ${UTILS_LIBRARIES})

ADD_LIBRARY(upnpframework STATIC
    inc/upnp/upnpaction.h               src/upnpaction.cpp
    inc/upnp/upnpavtransport.h          src/upnpavtransport.cpp
    inc/upnp/upnpclient.h               src/upnpclient.cpp
    inc/upnp/upnpconnectionmanager.h    src/upnpconnectionmanager.cpp
    inc/upnp/upnpcontentdirectory.h     src/upnpcontentdirectory.cpp
    inc/upnp/upnpcontrolpoint.h         src/upnpcontrolpoint.cpp
    inc/upnp/upnpdevice.h
    inc/upnp/upnpdevicescanner.h        src/upnpdevicescanner.cpp
    inc/upnp/upnphttpget.h              src/upnphttpget.cpp
    inc/upnp/upnpitem.h                 src/upnpitem.cpp
    inc/upnp/upnpmediarenderer.h        src/upnpmediarenderer.cpp
    inc/upnp/upnpmediaserver.h          src/upnpmediaserver.cpp
    inc/upnp/upnpprotocolinfo.h         src/upnpprotocolinfo.cpp
    inc/upnp/upnprenderingcontrol.h     src/upnprenderingcontrol.cpp
    inc/upnp/upnpxmlutils.h             src/upnpxmlutils.cpp
)

INCLUDE_DIRECTORIES(${UPNPFRAMEWORK_INCLUDE_DIRS})
LINK_DIRECTORIES(${UPNPFRAMEWORK_LIBRARY_DIRS})

ADD_DEFINITIONS("-std=c++11")
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    ADD_DEFINITIONS("-stdlib=libc++")
ENDIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

IF (UNIX OR APPLE)
    ADD_DEFINITIONS("-DCONSOLE_SUPPORTS_COLOR")
ENDIF (UNIX OR APPLE)

#ADD_CUSTOM_COMMAND(TARGET upnpframework
#                   POST_BUILD
#                   COMMAND ctags -R -f .tags .
#                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                   COMMENT "Generating tags" VERBATIM)

#ADD_SUBDIRECTORY(test)
