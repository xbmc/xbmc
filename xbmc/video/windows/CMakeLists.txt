set(SOURCES GUIWindowFullScreen.cpp
            GUIWindowVideoBase.cpp
            GUIWindowVideoNav.cpp
            GUIWindowVideoPlaylist.cpp
            VideoFileItemListModifier.cpp)

set(HEADERS GUIWindowFullScreen.h
            GUIWindowVideoBase.h
            GUIWindowVideoNav.h
            GUIWindowVideoPlaylist.h
            VideoFileItemListModifier.h)

core_add_library(video_windows)
