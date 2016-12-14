set(SOURCES AddonCallbacksGame.cpp)

set(HEADERS AddonCallbacksGame.h)

core_add_library(api1AddonCallbacks_Game)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api1AddonCallbacks_Game ffmpeg)
endif()
