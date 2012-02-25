CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP libupnp REQUIRED)

SET (UPNPFRAMEWORK_INCLUDE_DIRS ${UPNP_INCLUDE_DIRS})
SET (UPNPFRAMEWORK_LIBRARY_DIRS ${UPNP_LIBRARY_DIRS})
SET (UPNPFRAMEWORK_LIBRARIES ${UPNP_LIBRARIES})

ADD_LIBRARY(upnpframework STATIC
    upnp/upnpbrowser.cpp
    upnp/upnpbrowser.h
    upnp/upnpclient.cpp
    upnp/upnpclient.h
    upnp/upnpcontrolpoint.cpp
    upnp/upnpcontrolpoint.h
    upnp/upnpdevice.h
    upnp/upnpdevicescanner.cpp
    upnp/upnpdevicescanner.h
    upnp/upnphttpget.cpp
    upnp/upnphttpget.h
    upnp/upnpitem.cpp
    upnp/upnpitem.h
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}")
SET (CMAKE_CXX_FLAGS "-std=c++0x")
SET (CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Werror -Wfatal-errors -Wno-deprecated-declarations")