set(SOURCES CocoaInterface.mm
            CPUInfoOsx.cpp
            GPUInfoMacOS.cpp
            HotKeyController.mm
            MediaKeys.mm
            PlatformDarwinOSX.mm)

set(HEADERS CocoaInterface.h
            CPUInfoOsx.h
            GPUInfoMacOS.h
            HotKeyController.h
            MediaKeys.h
            PlatformDarwinOSX.h)

if(ENABLE_XBMCHELPER)
  list(APPEND SOURCES XBMCHelper.cpp)
  list(APPEND HEADERS XBMCHelper.h)
endif()

core_add_library(platform_osx)
