set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            VideoSyncOsx.h)

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemOSXGL.mm)
  list(APPEND HEADERS WinSystemOSXGL.h)
endif()

core_add_library(windowing_osx)
