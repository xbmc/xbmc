set(SOURCES OMXImage.cpp)

set(HEADERS OMXImage.h)

core_add_library(omxplayer)
target_compile_definitions(${CORE_LIBRARY} PRIVATE -D__STDC_FORMAT_MACROS)
