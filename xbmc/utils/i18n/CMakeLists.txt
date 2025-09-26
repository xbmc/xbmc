set(HEADERS TableISO3166_1.h
            TableISO639.h
            TableISO639_1.h
            TableISO639_2.h
            TableLanguageCodes.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(utils_i18n)
endif()
