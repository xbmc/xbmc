set(SOURCES Application.cpp
            AppInboundProtocol.cpp
            ApplicationPlayer.cpp
            ApplicationStackHelper.cpp
            AppParamParser.cpp
            AutoSwitch.cpp
            BackgroundInfoLoader.cpp
            ContextMenuItem.cpp
            ContextMenuManager.cpp
            ContextMenus.cpp
            CueDocument.cpp
            DatabaseManager.cpp
            DbUrl.cpp
            DynamicDll.cpp
            FileItem.cpp
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
            XBApplicationEx.cpp
            XBDateTime.cpp)

set(HEADERS AppParamParser.h
            Application.h
            AppInboundProtocol.h
            ApplicationPlayer.h
            ApplicationStackHelper.h
            AutoSwitch.h
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
            FileItemListModification.h
            GUIInfoManager.h
            GUILargeTextureManager.h
            GUIPassword.h
            GUIUserMessages.h
            IFileItemListModifier.h
            IProgressCallback.h
            InfoScanner.h
            LangInfo.h
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
            XBApplicationEx.h
            XBDateTime.h
            system.h
            system_gl.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES Autorun.cpp)
  list(APPEND HEADERS Autorun.h)
endif()

core_add_library(xbmc)
if(CAP_FOUND)
  target_compile_definitions(${CORE_LIBRARY} PRIVATE -DHAVE_LIBCAP=1)
endif()

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(${CORE_LIBRARY} PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
