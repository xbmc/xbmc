set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp
            VideoSettings.cpp)

set(HEADERS DataCacheCore.h
            FFmpeg.h
            GameSettings.h
            IPlayer.h
            IPlayerCallback.h
            VideoSettings.h)

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  add_subdirectory(omxplayer)
endif()

core_add_library(cores)
