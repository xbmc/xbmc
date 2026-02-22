set(SOURCES Bcp47.cpp
            Bcp47Formatter.cpp
            Bcp47Parser.cpp
            Iso3166_1.cpp
            Iso639.cpp
            Iso639_1.cpp
            Iso639_2.cpp)

set(HEADERS Bcp47.h
            Bcp47Common.h
            Bcp47Formatter.h
            Bcp47Parser.h
            Bcp47SubTags.h
            Iso3166_1.h
            Iso3166_1_Table.h
            Iso639.h
            Iso639_1.h
            Iso639_1_Table.h
            Iso639_2.h
            Iso639_2_Table.h
            TableLanguageCodes.h)

core_add_library(utils_i18n)
