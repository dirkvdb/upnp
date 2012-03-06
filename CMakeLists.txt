CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_SUBDIRECTORY(modules/utils)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP libupnp REQUIRED)

SET (UPNPFRAMEWORK_INCLUDE_DIRS ${UPNP_INCLUDE_DIRS})
SET (UPNPFRAMEWORK_LIBRARY_DIRS ${UPNP_LIBRARY_DIRS})
SET (UPNPFRAMEWORK_LIBRARIES ${UPNP_LIBRARIES})

ADD_LIBRARY(upnpframework STATIC
    inc/upnp/upnpbrowser.h          src/upnpbrowser.cpp
    inc/upnp/upnpclient.h           src/upnpclient.cpp
    inc/upnp/upnpcontrolpoint.h     src/upnpcontrolpoint.cpp
    inc/upnp/upnpdevice.h
    inc/upnp/upnpdevicescanner.h    src/upnpdevicescanner.cpp
    inc/upnp/upnphttpget.h          src/upnphttpget.cpp
    inc/upnp/upnpitem.h             src/upnpitem.cpp
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/inc/"
                    "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/inc/")

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
