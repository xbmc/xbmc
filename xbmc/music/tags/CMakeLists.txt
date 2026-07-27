set(SOURCES MusicCodecInfoFFmpeg.cpp
            MusicInfoTag.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp
            TagLibVFSStream.cpp
            TagLoaderTagLib.cpp)

set(HEADERS ImusicInfoTagLoader.h
            MusicCodecInfoFFmpeg.h
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

if(TAGLIB_HAS_MATROSKA)
  list(APPEND SOURCES MusicInfoTagLoaderMatroska.cpp)
  list(APPEND HEADERS MatroskaTagLibStream.h
                      MusicInfoTagLoaderMatroska.h)
endif()

core_add_library(music_tags)
