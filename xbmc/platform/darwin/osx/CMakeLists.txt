set(SOURCES CocoaInterface.mm
            CPUInfoOsx.cpp
            HotKeyController.m
            PlatformDarwinOSX.cpp
            smc.c)

set(HEADERS CocoaInterface.h
            CPUInfoOsx.h
            HotKeyController.h
            PlatformDarwinOSX.h
            smc.h)

if(ENABLE_XBMCHELPER)
  list(APPEND SOURCES XBMCHelper.cpp)
  list(APPEND HEADERS XBMCHelper.h)
endif()

core_add_library(platform_osx)
