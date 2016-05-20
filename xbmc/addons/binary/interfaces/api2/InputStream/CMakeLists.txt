set(SOURCES Addon_InputStream.cpp)

set(HEADERS Addon_InputStream.h
            ../../../../kodi-addon-dev-kit/src/api2/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api2AddonInterfaces_InputStream)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api2AddonInterfaces_InputStream ffmpeg)
endif()
