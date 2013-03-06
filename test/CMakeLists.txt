CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ENABLE_TESTING()

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(
	${UPNPFRAMEWORK_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

ADD_EXECUTABLE(upnptest
	gmock-gtest-all.cc
	devicediscover.h
	main.cpp
    xmlutilstest.cpp
	#mediarenderertest.cpp
	#mediaservertest.cpp
	#controlpointtest.cpp
	testenvironment.h
)

SET (CMAKE_CXX_FLAGS "-std=c++11")

LINK_DIRECTORIES(
    ${UPNPFRAMEWORK_LIBRARY_DIRS}
)

TARGET_LINK_LIBRARIES(upnptest
	-lpthread
	${UPNPFRAMEWORK_LIBRARIES}
)

ADD_TEST(UPnPTests upnptest)
 
