set(SOURCES Addon_Player.cpp
            Addon_PlayList.cpp
            Addon_InfoTagMusic.cpp
            Addon_InfoTagVideo.cpp)

set(HEADERS Addon_Player.h
            Addon_PlayList.h
            Addon_InfoTagMusic.h
            Addon_InfoTagVideo.h
            ../../../../kodi-addon-dev-kit/src/api2/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api2AddonInterfaces_Player)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api2AddonInterfaces_Player ffmpeg)
endif()
