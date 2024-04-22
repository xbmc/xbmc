set(SOURCES AutoSwitch.cpp
            BackgroundInfoLoader.cpp
            ContextMenuItem.cpp
            ContextMenuManager.cpp
            ContextMenus.cpp
            CueDocument.cpp
            DatabaseManager.cpp
            DbUrl.cpp
            DynamicDll.cpp
            FileItem.cpp
            FileItemList.cpp
            FileItemListModification.cpp
            GUIInfoManager.cpp
            GUILargeTextureManager.cpp
            GUIPassword.cpp
            InfoScanner.cpp
            LangInfo.cpp
            MediaSource.cpp
            NfoFile.cpp
            PasswordManager.cpp
            PlayListPlayer.cpp
            PartyModeManager.cpp
            SectionLoader.cpp
            SeekHandler.cpp
            ServiceBroker.cpp
            ServiceManager.cpp
            SystemGlobals.cpp
            TextureCache.cpp
            TextureCacheJob.cpp
            TextureDatabase.cpp
            ThumbLoader.cpp
            URL.cpp
            Util.cpp
            XBDateTime.cpp)

set(HEADERS AutoSwitch.h
            BackgroundInfoLoader.h
            CompileInfo.h
            ContextMenuItem.h
            ContextMenuManager.h
            ContextMenus.h
            CueDocument.h
            DatabaseManager.h
            DbUrl.h
            DllPaths.h
            DllPaths_win32.h
            DynamicDll.h
            FileItem.h
            FileItemList.h
            FileItemListModification.h
            GUIInfoManager.h
            GUILargeTextureManager.h
            GUIPassword.h
            GUIUserMessages.h
            HDRStatus.h
            IFileItemListModifier.h
            IProgressCallback.h
            InfoScanner.h
            LangInfo.h
            LockType.h
            MediaSource.h
            NfoFile.h
            PartyModeManager.h
            PasswordManager.h
            PlayListPlayer.h
            SectionLoader.h
            SeekHandler.h
            ServiceBroker.h
            ServiceManager.h
            SortFileItem.h
            TextureCache.h
            TextureCacheJob.h
            TextureDatabase.h
            ThumbLoader.h
            URL.h
            Util.h
            XBDateTime.h
            system_egl.h
            system_gl.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES Autorun.cpp)
  list(APPEND HEADERS Autorun.h)
endif()

core_add_library(xbmc)

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(${CORE_LIBRARY} PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
