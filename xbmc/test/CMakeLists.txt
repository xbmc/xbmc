set(TEST_SRCS
	TestBasicEnvironment.cpp
	TestBasicEnvironment.h
	TestUtils.cpp
	TestUtils.h
	xbmc-test.cpp
)

include_directories(${root}/lib/gtest ${root}/lib/gtest/include)

if(ENABLE_TESTING)
	add_library(xbmctest OBJECT ${TEST_SRCS})
endif(ENABLE_TESTING)