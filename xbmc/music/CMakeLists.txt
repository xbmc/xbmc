set(SOURCES Album.cpp
            Artist.cpp
            ContextMenus.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
            MusicEmbeddedCoverLoaderFFmpeg.cpp
            MusicEmbeddedImageFileLoader.cpp
            MusicFileItemClassify.cpp
            MusicInfoLoader.cpp
            MusicLibraryQuery.cpp
            MusicLibraryQueue.cpp
            MusicThumbLoader.cpp
            MusicUtils.cpp
            Song.cpp)

set(HEADERS Album.h
            Artist.h
            AudioType.h
            ContextMenus.h
            GUIViewStateMusic.h
            MusicDatabase.h
            MusicDbPaths.h
            MusicDbUrl.h
            MusicEmbeddedCoverLoaderFFmpeg.h
            MusicEmbeddedImageFileLoader.h
            MusicFileItemClassify.h
            MusicInfoLoader.h
            MusicLibraryQuery.h
            MusicLibraryQueue.h
            MusicThumbLoader.h
            MusicUtils.h
            Song.h)

core_add_library(music)
