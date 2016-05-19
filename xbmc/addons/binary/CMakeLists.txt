set(SOURCES AddonInterfaceManager.cpp
            ExceptionHandling.cpp)

set(HEADERS AddonInterfaceManager.h
            ExceptionHandling.h)

core_add_library(addonsBinary)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(addonsBinary ffmpeg)
endif()
