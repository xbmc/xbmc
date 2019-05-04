set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp
            VideoSettings.cpp)

set(HEADERS DataCacheCore.h
            Cut.h
            FFmpeg.h
            GameSettings.h
            IPlayer.h
            IPlayerCallback.h
            VideoSettings.h)

core_add_library(cores)
