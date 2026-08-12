set(SOURCES MatroskaTagMapping.cpp
            MatroskaTagReader.cpp
            MusicCodecInfoFFmpeg.cpp
            MusicInfoTag.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderMatroska.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp
            TagLibVFSStream.cpp
            TagLoaderTagLib.cpp)

set(HEADERS ImusicInfoTagLoader.h
            MatroskaTagMapping.h
            MatroskaTagReader.h
            MusicCodecInfoFFmpeg.h
            MusicInfoTag.h
            MusicInfoTagLoaderDatabase.h
            MusicInfoTagLoaderFactory.h
            MusicInfoTagLoaderFFmpeg.h
            MusicInfoTagLoaderMatroska.h
            MusicInfoTagLoaderShn.h
            MatroskaTagLibStream.h
            ReplayGain.h
            TagLibVersion.h
            TagLibVFSStream.h
            TagLoaderTagLib.h)
if(ENABLE_OPTICAL)
  list(APPEND SOURCES MusicInfoTagLoaderCDDA.cpp)
  list(APPEND HEADERS MusicInfoTagLoaderCDDA.h)
endif()

core_add_library(music_tags)
