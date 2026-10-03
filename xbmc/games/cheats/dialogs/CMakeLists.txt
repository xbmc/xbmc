set(SOURCES DialogGameCheats.cpp)
set(HEADERS DialogGameCheats.h)

core_add_library(games_cheats_dialogs)

if(ENABLE_TESTING)
  add_subdirectory(test)
endif()
