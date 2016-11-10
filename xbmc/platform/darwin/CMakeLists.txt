set(SOURCES AutoPool.mm
            DarwinInterfaceForCLog.mm
            DarwinUtils.mm
            DarwinNSUserDefaults.mm
            DictionaryUtils.mm
            OSXGNUReplacements.c
            PlatformDarwin.cpp)

set(HEADERS AutoPool.h
            DarwinInterfaceForCLog.h
            DarwinNSUserDefaults.h
            DarwinUtils.h
            DictionaryUtils.h
            NSLogDebugHelpers.h
            OSXGNUReplacements.h
            PlatformDarwin.h)

core_add_library(platform_darwin)
