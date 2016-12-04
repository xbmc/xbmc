set(SOURCES AddonCallbacksGame.cpp)

set(HEADERS AddonCallbacksGame.h)

core_add_library(addonInterface_Game)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(addonInterface_Game ffmpeg)
endif()
