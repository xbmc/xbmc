set(SOURCES OSScreenSaver.cpp
            WinEvents.cpp
            WinSystem.cpp)

set(HEADERS OSScreenSaver.h
            WindowingFactory.h
            WinEvents.h
            WinSystem.h
            XBMC_events.h
            VideoSync.h)

if((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL rbpi) AND NOT MIR_FOUND)
  list(APPEND SOURCES WinEventsLinux.cpp)
  list(APPEND HEADERS WinEventsLinux.h)
endif()

core_add_library(windowing)
