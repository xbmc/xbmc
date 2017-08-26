set(SOURCES "")
set(HEADERS "")

if(DBUS_FOUND)
  list(APPEND SOURCES OSScreenSaverFreedesktop.cpp)
  list(APPEND HEADERS OSScreenSaverFreedesktop.h)
endif()

if(SOURCES)
  core_add_library(windowing_linux)
endif()