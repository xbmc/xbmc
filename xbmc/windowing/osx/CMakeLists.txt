if(CORE_SYSTEM_NAME STREQUAL osx)
  set(SOURCES WinEventsOSX.mm
              WinSystemOSX.mm
              WinSystemOSXGL.mm)
  set(HEADERS WinEventsOSX.h
              WinSystemOSX.h
              WinSystemOSXGL.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  set(SOURCES WinEventsIOS.mm
              WinSystemIOS.mm)
  set(HEADERS WinEventsIOS.h
              WinSystemIOS.h)
endif()

core_add_library(windowing_osx)
