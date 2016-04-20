set(SOURCES AddonInterfaceBase.cpp)

set(HEADERS AddonInterfaceBase.h
            ../../../kodi-addon-dev-kit/src/api3/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api3AddonInterfaces)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api3AddonInterfaces ffmpeg)
endif()
