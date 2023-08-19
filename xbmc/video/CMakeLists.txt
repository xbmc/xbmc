set(SOURCES Bookmark.cpp
            ContextMenus.cpp
            GUIViewStateVideo.cpp
            PlayerController.cpp
            Teletext.cpp
            VideoChapterImageFileLoader.cpp
            VideoDatabase.cpp
            VideoDbUrl.cpp
            VideoEmbeddedImageFileLoader.cpp
            VideoGeneratedImageFileLoader.cpp
            VideoInfoDownloader.cpp
            VideoInfoScanner.cpp
            VideoInfoTag.cpp
            VideoItemArtworkHandler.cpp
            VideoLibraryQueue.cpp
            VideoThumbLoader.cpp
            VideoUtils.cpp
            ViewModeSettings.cpp)

set(HEADERS Bookmark.h
            ContextMenus.h
            Episode.h
            GUIViewStateVideo.h
            PlayerController.h
            Teletext.h
            TeletextDefines.h
            VideoChapterImageFileLoader.h
            VideoDatabase.h
            VideoDbUrl.h
            VideoEmbeddedImageFileLoader.h
            VideoGeneratedImageFileLoader.h
            VideoInfoDownloader.h
            VideoInfoScanner.h
            VideoInfoTag.h
            VideoItemArtworkHandler.h
            VideoLibraryQueue.h
            VideoThumbLoader.h
            VideoUtils.h
            ViewModeSettings.h)

core_add_library(video)
