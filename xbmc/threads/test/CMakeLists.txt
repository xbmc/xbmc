set(SOURCES TestEvent.cpp
            TestSharedSection.cpp
            TestAtomics.cpp
            TestThreadLocal.cpp)

set(HEADERS TestHelpers.h)

core_add_test_library(threads_test)
target_include_directories(threads_test
                           PRIVATE ${CORE_SOURCE_DIR}/lib/gtest/include)
