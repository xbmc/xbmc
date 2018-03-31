set(HEADERS AddonBase.h
            AudioEngine.h
            Filesystem.h
            General.h
            Network.h
            StreamCodec.h
            StreamCrypto.h
            kodi_game_dll.h
            kodi_game_types.h
            kodi_vfs_types.h
            libKODI_game.h
            libKODI_guilib.h
            libXBMC_addon.h
            libXBMC_pvr.h
            versions.h
            xbmc_addon_dll.h
            xbmc_addon_types.h
            xbmc_epg_types.h
            xbmc_pvr_dll.h
            xbmc_pvr_types.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES platform/android/System.h)
endif()

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi)
endif()
