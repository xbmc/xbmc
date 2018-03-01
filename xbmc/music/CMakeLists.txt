set(SOURCES Album.cpp
            Artist.cpp
            ContextMenus.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
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
            MusicInfoLoader.h
            MusicLibraryQueue.h
            MusicThumbLoader.h
            MusicUtils.h
            Song.h)

core_add_library(music)
