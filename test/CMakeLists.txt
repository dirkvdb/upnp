INCLUDE_DIRECTORIES(
    ${UTILS_INCLUDE_DIRS}
	${UPNPFRAMEWORK_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${UPNPFRAMEWORK_LIBRARY_DIRS}
)

ADD_EXECUTABLE(upnptest
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
    upnpframework
    utils
)

ADD_TEST(NAME UPnPTests COMMAND upnptest)
