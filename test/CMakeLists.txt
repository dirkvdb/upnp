CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

GET_DIRECTORY_PROPERTY(UTILS_INCLUDE_DIRS	DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../utils" DEFINITION UTILS_INCLUDE_DIRS)
GET_DIRECTORY_PROPERTY(UTILS_LIBRARIES      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../utils" DEFINITION UTILS_LIBRARIES)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(
    ${UTILS_INCLUDE_DIRS}
	${UPNPFRAMEWORK_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${UPNPFRAMEWORK_LIBRARY_DIRS}
)

IF (BUILD_TESTING)
    ADD_EXECUTABLE(upnptest
        gmock-gtest-all.cpp
        main.cpp

        devicediscover.h

        xmlutilstest.cpp
        #mediarenderertest.cpp
        #mediaservertest.cpp
        #controlpointtest.cpp
        testenvironment.h
        webservertest.cpp
        upnpavtransporttest.cpp
        upnpcontentdirectorytest.cpp
        upnprenderingcontroltest.cpp
        upnpservicebasetest.cpp
    )


    TARGET_LINK_LIBRARIES(upnptest
        -lpthread
        ${UTILS_LIBRARIES}
        ${UPNPFRAMEWORK_LIBRARIES}
    )

    ADD_TEST(UPnPTests upnptest)
ENDIF (BUILD_TESTING)