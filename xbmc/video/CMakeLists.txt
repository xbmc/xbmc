set(SOURCES Bookmark.cpp
            FFmpegVideoDecoder.cpp
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
            Episode.h
            FFmpegVideoDecoder.h
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
