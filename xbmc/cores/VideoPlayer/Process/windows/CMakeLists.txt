if(CORE_SYSTEM_NAME STREQUAL windows)
  set(SOURCES ProcessInfoWin.cpp)
  set(HEADERS ProcessInfoWin.h)
elseif(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set(SOURCES ProcessInfoWin10.cpp)
  set(HEADERS ProcessInfoWin10.h)
endif()

core_add_library(processwin)
