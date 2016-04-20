set(SOURCES Addon_AudioEngineGeneral.cpp
            Addon_AudioEngineStream.cpp)

set(HEADERS Addon_AudioEngineGeneral.h
            Addon_AudioEngineStream.h
            ../../../../kodi-addon-dev-kit/src/api2/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api2AddonInterfaces_AudioEngine)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api2AddonInterfaces_AudioEngine ffmpeg)
endif()
