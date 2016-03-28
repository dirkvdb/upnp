INCLUDE_DIRECTORIES(
    ${UTILS_INCLUDE_DIRS}
    ${UPNPFRAMEWORK_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

INCLUDE_DIRECTORIES(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/testframework
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

ADD_EXECUTABLE(upnpframeworktest
    testrunner.cpp
    upnpssdpclienttest.cpp
    upnphttpclienttest.cpp
    upnpssdptest.cpp
    upnpdevicescannertest.cpp
)

TARGET_LINK_LIBRARIES(upnptest
    upnpframework
)

TARGET_LINK_LIBRARIES(upnpframeworktest
    upnpframework
)

ADD_TEST(NAME UPnPTests COMMAND upnptest)
ADD_TEST(NAME UPnPFrameworkTests COMMAND upnpframeworktest)
