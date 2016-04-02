set(SOURCES Bookmark.cpp
            ContextMenus.cpp
            GUIViewStateVideo.cpp
            PlayerController.cpp
            Teletext.cpp
            VideoDatabase.cpp
            VideoDbUrl.cpp
            VideoInfoDownloader.cpp
            VideoInfoScanner.cpp
            VideoInfoTag.cpp
            VideoLibraryQueue.cpp
            VideoReferenceClock.cpp
            VideoThumbLoader.cpp)

set(HEADERS Bookmark.h
            ContextMenus.h
            Episode.h
            GUIViewStateVideo.h
            PlayerController.h
            Teletext.h
            TeletextDefines.h
            VideoDatabase.h
            VideoDbUrl.h
            VideoInfoDownloader.h
            VideoInfoScanner.h
            VideoInfoTag.h
            VideoLibraryQueue.h
            VideoReferenceClock.h
            VideoThumbLoader.h)

core_add_library(video)
add_dependencies(video libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(video ffmpeg)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(video d3dx11effects)
endif()
