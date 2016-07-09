set(SOURCES MusicInfoTag.cpp
            MusicInfoTagLoaderCDDA.cpp
            MusicInfoTagLoaderDatabase.cpp
            MusicInfoTagLoaderFactory.cpp
            MusicInfoTagLoaderFFmpeg.cpp
            MusicInfoTagLoaderShn.cpp
            ReplayGain.cpp
            TagLibVFSStream.cpp
            TagLoaderTagLib.cpp)

set(HEADERS ImusicInfoTagLoader.h
            MusicInfoTag.h
            MusicInfoTagLoaderCDDA.h
            MusicInfoTagLoaderDatabase.h
            MusicInfoTagLoaderFactory.h
            MusicInfoTagLoaderFFmpeg.h
            MusicInfoTagLoaderShn.h
            ReplayGain.h
            TagLibVFSStream.h
            TagLoaderTagLib.h)

core_add_library(music_tags)
