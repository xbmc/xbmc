set(SOURCES CocoaInterface.mm
            CPUInfoOsx.cpp
            HotKeyController.m
            PlatformDarwinOSX.cpp
            smc.c
            XBMCHelper.cpp)

set(HEADERS CocoaInterface.h
            CPUInfoOsx.h
            HotKeyController.h
            PlatformDarwinOSX.h
            smc.h
            XBMCHelper.h)

if(NOT SDL_FOUND)
  list(APPEND SOURCES OSXGLView.mm
                      OSXGLWindow.mm)
  list(APPEND HEADERS OSXGLView.h
                      OSXGLWindow.h)
endif()

core_add_library(platform_osx)
