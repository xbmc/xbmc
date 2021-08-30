set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            WinEventsOSX.mm
            WinEventsSDL.cpp
            WinSystemOSX.mm
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            WinEventsOSX.h
            WinEventsSDL.h
            WinSystemOSX.h
            VideoSyncOsx.h)

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemOSXGL.mm)
  list(APPEND HEADERS WinSystemOSXGL.h)
endif()

core_add_library(windowing_osx)
