set(SOURCES Album.cpp
            Artist.cpp
            ContextMenus.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
            MusicEmbeddedImageFileLoader.cpp
            MusicFileItemClassify.cpp
            MusicInfoLoader.cpp
            MusicLibraryQueue.cpp
            MusicThumbLoader.cpp
            MusicUtils.cpp
            Song.cpp)

set(HEADERS Album.h
            Artist.h
            ContextMenus.h
            GUIViewStateMusic.h
            MusicDatabase.h
            MusicDbUrl.h
            MusicEmbeddedImageFileLoader.h
            MusicFileItemClassify.h
            MusicInfoLoader.h
            MusicLibraryQueue.h
            MusicThumbLoader.h
            MusicUtils.h
            Song.h)

core_add_library(music)
