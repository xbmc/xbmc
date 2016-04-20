set(SOURCES Addon_PVR.cpp)

set(HEADERS Addon_PVR.h
            ../../../../kodi-addon-dev-kit/src/api3/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api3AddonInterfaces_PVR)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api3AddonInterfaces_PVR ffmpeg)
endif()
