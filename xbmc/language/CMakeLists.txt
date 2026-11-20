set(SOURCES LangInfo.cpp
            Language.cpp
            LanguageLoader.cpp
            LanguageTag.cpp
            Territory.cpp)

set(HEADERS LangInfo.h
            Language.h
            LanguageLoader.h
            LanguageTag.h
            Territory.h)

core_add_library(language)
