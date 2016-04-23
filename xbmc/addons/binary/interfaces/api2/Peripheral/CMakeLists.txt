set(SOURCES Addon_Peripheral.cpp)

set(HEADERS Addon_Peripheral.h
            ../../../../kodi-addon-dev-kit/src/api2/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api2AddonInterfaces_Peripheral)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api2AddonInterfaces_Peripheral ffmpeg)
endif()
