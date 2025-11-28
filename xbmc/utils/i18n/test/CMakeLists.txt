set(SOURCES TestBcp47.cpp
            TestBcp47Formatter.cpp
            TestBcp47Parser.cpp
            TestIso3166_1.cpp
            TestIso639_1.cpp
            TestIso639_2.cpp
            TestUtils.cpp)

set(HEADERS TestUtils.h)

core_add_test_library(utils_i18n_test)
