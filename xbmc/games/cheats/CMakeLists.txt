set(SOURCES CheatPack.cpp
            CheatUtils.cpp)

set(HEADERS CheatPack.h
            CheatUtils.h)

core_add_library(games_cheats)

if(ENABLE_TESTING)
  add_subdirectory(test)
endif()
