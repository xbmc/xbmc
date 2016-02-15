set(SOURCES PlayerCoreFactory.cpp
            PlayerSelectionRule.cpp)

set(HEADERS PlayerCoreConfig.h
            PlayerCoreFactory.h
            PlayerSelectionRule.h)

core_add_library(playercorefactory)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(playercorefactory ffmpeg)
endif()
