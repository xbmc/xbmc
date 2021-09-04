set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            VideoSyncOsx.h)

if(NOT SDL_FOUND)
  list(APPEND SOURCES WinEventsOSX.mm
                      WinEventsOSXImpl.mm)
  list(APPEND HEADERS WinEventsOSX.h
                      WinEventsOSXImpl.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemOSXGL.mm)
  list(APPEND HEADERS WinSystemOSXGL.h)
endif()

core_add_library(windowing_osx)
