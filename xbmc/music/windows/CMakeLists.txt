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
add_dependencies(music_windows libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(music_windows ffmpeg)
endif()
