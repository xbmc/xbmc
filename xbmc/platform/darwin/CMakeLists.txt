set(SOURCES DarwinUtils.mm
            DictionaryUtils.mm
            MemUtils.cpp
            PlatformDarwin.cpp)

set(HEADERS DarwinUtils.h
            DictionaryUtils.h
            NSLogDebugHelpers.h
            PlatformDarwin.h)

set(UNITY_EXCLUDES DarwinUtils.mm
                   DictionaryUtils.mm)

core_add_library(platform_darwin)
