set(SOURCES Album.cpp
            Artist.cpp
            ContextMenus.cpp
            CueInfoLoader.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
            MusicInfoLoader.cpp
            MusicThumbLoader.cpp
            Song.cpp)

set(HEADERS Album.h
            Artist.h
            ContextMenus.h
            CueInfoLoader.h
            EmbeddedArt.h
            GUIViewStateMusic.h
            MusicDatabase.h
            MusicDbUrl.h
            MusicInfoLoader.h
            MusicThumbLoader.h
            Song.h)

core_add_library(music)
