set(SOURCES AutoPool.mm
            DarwinUtils.mm
            DarwinNSUserDefaults.mm
            DictionaryUtils.mm
            PlatformDarwin.cpp)

set(HEADERS AutoPool.h
            DarwinNSUserDefaults.h
            DarwinUtils.h
            DictionaryUtils.h
            NSLogDebugHelpers.h
            PlatformDarwin.h)

core_add_library(platform_darwin)
