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
add_dependencies(video_windows libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(video_windows ffmpeg)
endif()
