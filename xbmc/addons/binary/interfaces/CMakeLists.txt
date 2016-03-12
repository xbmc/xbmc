set(SOURCES AddonInterfaces.cpp)

set(HEADERS AddonInterfaces.h
            IAddonInterface.h)

core_add_library(addonsBinaryInterfaces)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(addonsBinaryInterfaces ffmpeg)
endif()
