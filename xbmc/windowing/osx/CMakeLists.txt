set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            VideoSyncOsx.h)

if(NOT SDL_FOUND)
  list(APPEND SOURCES WinEventsOSX.mm
                      WinEventsOSXImpl.mm
                      WinSystemOSX.mm)
  list(APPEND HEADERS WinEventsOSX.h
                      WinEventsOSXImpl.h
                      WinSystemOSX.h)
endif()

core_add_library(windowing_osx)
