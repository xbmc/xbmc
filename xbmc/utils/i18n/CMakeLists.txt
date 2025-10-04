set(SOURCES Iso639_1.cpp
            Iso639_2.cpp
            Iso639_3.cpp)

set(HEADERS Iso639_1.h
            Iso639_1_Table.h
            Iso639_2.h
            Iso639_2_Table.h
            TableISO3166_1.h
            TableISO639.h
            Iso639_3.h
            Iso639_3_Table.h
            TableLanguageCodes.h)

core_add_library(utils_i18n)
