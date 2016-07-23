set(SOURCES TestEvent.cpp
            TestSharedSection.cpp
            TestAtomics.cpp
            TestThreadLocal.cpp)

set(HEADERS TestHelpers.h)

core_add_test_library(threads_test)
