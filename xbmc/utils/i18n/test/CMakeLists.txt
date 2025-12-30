set(SOURCES TestBcp47.cpp
            TestBcp47Formatter.cpp
            TestBcp47Parser.cpp
            TestIso3166_1.cpp
            TestIso639_1.cpp
            TestIso639_2.cpp
            TestI18nUtils.cpp)

set(HEADERS TestI18nUtils.h)

core_add_test_library(utils_i18n_test)
