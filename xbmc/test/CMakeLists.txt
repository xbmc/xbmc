set(SOURCES TestBasicEnvironment.cpp
            TestFileItem.cpp
            TestTextureUtils.cpp
            TestURL.cpp
            TestUtil.cpp
            TestUtils.cpp
            TestDateTime.cpp
            TestDateTimeSpan.cpp)

set(HEADERS TestBasicEnvironment.h
            TestUtils.h)

core_add_test_library(xbmc_test)
