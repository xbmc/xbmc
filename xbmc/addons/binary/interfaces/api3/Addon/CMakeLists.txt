set(SOURCES Addon_General.cpp
            Addon_Audio.cpp
            Addon_Directory.cpp
            Addon_File.cpp
            Addon_Network.cpp)

set(HEADERS Addon_General.h
            Addon_Audio.h
            Addon_Directory.h
            Addon_File.h
            Addon_Network.h
            ../../../../kodi-addon-dev-kit/src/api3/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api3AddonInterfaces_Addon)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api3AddonInterfaces_Addon ffmpeg)
endif()
