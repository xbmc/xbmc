set(SOURCES MusicInfoTag.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp
            TagLibVFSStream.cpp
            TagLoaderTagLib.cpp)

set(HEADERS ImusicInfoTagLoader.h
            MusicInfoTag.h
            MusicInfoTagLoaderDatabase.h
            MusicInfoTagLoaderFactory.h
            MusicInfoTagLoaderFFmpeg.h
            MusicInfoTagLoaderShn.h
            ReplayGain.h
            TagLibVFSStream.h
            TagLoaderTagLib.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES MusicInfoTagLoaderCDDA.cpp)
  list(APPEND HEADERS MusicInfoTagLoaderCDDA.h)
endif()

core_add_library(music_tags)
