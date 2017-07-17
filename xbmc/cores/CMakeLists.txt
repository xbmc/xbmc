set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp)

set(HEADERS DataCacheCore.h
            FFmpeg.h
            IPlayer.h
            IPlayerCallback.h)

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  add_subdirectory(omxplayer)
endif()

core_add_library(cores)
