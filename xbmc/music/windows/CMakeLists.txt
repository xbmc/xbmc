set(SOURCES GUIWindowMusicBase.cpp
            GUIWindowMusicNav.cpp
            GUIWindowMusicPlaylist.cpp
            GUIWindowMusicPlaylistEditor.cpp
            GUIWindowVisualisation.cpp
            MusicFileItemListModifier.cpp)

set(HEADERS GUIWindowMusicBase.h
            GUIWindowMusicNav.h
            GUIWindowMusicPlaylist.h
            GUIWindowMusicPlaylistEditor.h
            GUIWindowVisualisation.h
            MusicFileItemListModifier.h)

core_add_library(music_windows)
