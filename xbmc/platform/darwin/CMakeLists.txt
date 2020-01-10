set(SOURCES DarwinUtils.mm
            DictionaryUtils.mm
            MemUtils.cpp
            PlatformDarwin.cpp)

set(HEADERS DarwinUtils.h
            DictionaryUtils.h
            NSLogDebugHelpers.h
            PlatformDarwin.h)

core_add_library(platform_darwin)
