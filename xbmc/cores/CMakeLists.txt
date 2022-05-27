set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp
            VideoSettings.cpp)

set(HEADERS DataCacheCore.h
            EdlEdit.h
            FFmpeg.h
            GameSettings.h
            IPlayer.h
            IPlayerCallback.h
            MenuType.h
            VideoSettings.h)

core_add_library(cores)
