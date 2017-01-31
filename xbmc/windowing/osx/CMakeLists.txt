if(CORE_SYSTEM_NAME STREQUAL osx)
  set(SOURCES WinEventsOSX.mm
              WinSystemOSX.mm
              WinSystemOSXGL.mm
              VideoSyncOsx.cpp)
  set(HEADERS WinEventsOSX.h
              WinSystemOSX.h
              WinSystemOSXGL.h
              VideoSyncOsx.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  set(SOURCES WinEventsIOS.mm
              WinSystemIOS.mm
              VideoSyncIos.cpp)
  set(HEADERS WinEventsIOS.h
              WinSystemIOS.h
              VideoSyncIos.h)
endif()

core_add_library(windowing_osx)
