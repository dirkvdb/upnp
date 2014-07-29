CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE_DIRECTORIES(
    ${UTILS_INCLUDE_DIRS}
	${UPNPFRAMEWORK_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${UPNPFRAMEWORK_LIBRARY_DIRS}
)

ADD_EXECUTABLE(upnptest EXCLUDE_FROM_ALL
    gmock-gtest-all.cpp
    main.cpp

    devicediscover.h

    xmlutilstest.cpp
    upnputilstest.cpp
    #mediarenderertest.cpp
    #mediaservertest.cpp
    #controlpointtest.cpp
    testenvironment.h
    httpreadertest.cpp
    webservertest.cpp
    upnpavtransporttest.cpp
    upnpcontentdirectorytest.cpp
    upnprenderingcontroltest.cpp
    upnpservicebasetest.cpp
)


TARGET_LINK_LIBRARIES(upnptest
    ${UPNPFRAMEWORK_LIBRARIES}
)

ADD_TEST(NAME UPnPTests COMMAND upnptest)
ADD_DEPENDENCIES(check upnptest)