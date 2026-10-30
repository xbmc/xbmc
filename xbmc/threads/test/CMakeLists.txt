set(SOURCES TestEvent.cpp
            TestSharedSection.cpp
            TestEndTime.cpp
            TestTimer.cpp)

set(HEADERS TestHelpers.h)

core_add_test_library(threads_test)
