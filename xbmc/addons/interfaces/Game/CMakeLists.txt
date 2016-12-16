set(SOURCES AddonCallbacksGame.cpp)

set(HEADERS AddonCallbacksGame.h)

core_add_library(addonCallbacks_Game)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(${CORE_LIBRARY} ffmpeg)
endif()
