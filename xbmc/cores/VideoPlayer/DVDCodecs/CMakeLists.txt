set(SOURCES DVDCodecUtils.cpp
            DVDFactoryCodec.cpp)

set(HEADERS DVDCodecUtils.h
            DVDCodecs.h
            DVDFactoryCodec.h)

core_add_library(dvdcodecs)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdcodecs ffmpeg)
endif()
