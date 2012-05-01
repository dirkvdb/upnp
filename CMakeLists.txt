CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_SUBDIRECTORY(modules/utils)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(UPNP libupnp)

IF (UPNP_FOUND)
    SET (UPNPFRAMEWORK_INCLUDE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
            "${CMAKE_CURRENT_SOURCE_DIR}/modules/utils/inc/"
            ${UPNP_INCLUDE_DIRS})
    SET (UPNPFRAMEWORK_LIBRARY_DIRS ${UPNP_LIBRARY_DIRS})
    SET (UPNPFRAMEWORK_LIBRARIES upnpframework ${UPNP_LIBRARIES})

    ADD_LIBRARY(upnpframework STATIC
        inc/upnp/upnpaction.h               src/upnpaction.cpp
        inc/upnp/upnpclient.h               src/upnpclient.cpp
        inc/upnp/upnpconnectionmanager.h    src/upnpconnectionmanager.cpp
        inc/upnp/upnpcontentdirectory.h     src/upnpcontentdirectory.cpp
        #inc/upnp/upnpcontrolpoint.h         src/upnpcontrolpoint.cpp
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

    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
ENDIF (UPNP_FOUND)

ADD_SUBDIRECTORY(test)
