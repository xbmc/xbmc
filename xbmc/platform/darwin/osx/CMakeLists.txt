set(SOURCES CocoaInterface.mm
            HotKeyController.m
            OSXTextInputResponder.mm
            SDLMain.mm
            smc.c
            XBMCHelper.cpp)

set(HEADERS CocoaInterface.h
            HotKeyController.h
            OSXTextInputResponder.h
            SDLMain.h
            smc.h
            XBMCHelper.h)

core_add_library(platform_osx)
