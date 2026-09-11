set(SOURCES TestBcp47.cpp
            TestCollation.cpp
            TestBcp47Formatter.cpp
            TestBcp47Parser.cpp
            TestIso3166_1.cpp
            TestIso639.cpp
            TestIso639_2.cpp
            TestLanguageTable.cpp
            TestI18nUtils.cpp
            TestListFormatter.cpp)

set(HEADERS TestI18nUtils.h)

core_add_test_library(language_i18n_test)
