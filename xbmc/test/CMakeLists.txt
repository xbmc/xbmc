set(SOURCES TestAutoSwitch.cpp
            TestBasicEnvironment.cpp
            TestCueDocument.cpp
            TestDateTime.cpp
            TestDateTimeSpan.cpp
            TestFileItem.cpp
            TestMediaSource.cpp
            TestURL.cpp
            TestUtil.cpp
            TestUtils.cpp)

set(HEADERS TestBasicEnvironment.h
            TestUtils.h)

core_add_test_library(xbmc_test)
