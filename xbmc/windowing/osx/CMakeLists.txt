set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            WinEventsOSX.mm
            WinSystemOSX.mm
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            WinEventsOSX.h
            WinSystemOSX.h
            VideoSyncOsx.h)

if(SDL_FOUND)
  list(APPEND SOURCES WinEventsSDL.cpp)
  list(APPEND HEADERS WinEventsSDL.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemOSXGL.mm)
  list(APPEND HEADERS WinSystemOSXGL.h)
endif()

core_add_library(windowing_osx)
