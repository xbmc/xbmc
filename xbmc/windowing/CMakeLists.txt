set(SOURCES OSScreenSaver.cpp
            WinSystem.cpp)

set(HEADERS OSScreenSaver.h
            WindowingFactory.h
            WinEvents.h
            WinSystem.h
            XBMC_events.h
            VideoSync.h)

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi OR CORE_PLATFORM_NAME_LC STREQUAL gbm OR CORE_PLATFORM_NAME_LC STREQUAL aml)
  list(APPEND SOURCES WinEventsLinux.cpp)
  list(APPEND HEADERS WinEventsLinux.h)
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  add_subdirectory(rpi)
endif()

core_add_library(windowing)
