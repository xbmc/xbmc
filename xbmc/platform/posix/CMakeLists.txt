set(SOURCES ConvUtils.cpp
            Filesystem.cpp
            MessagePrinter.cpp
            PlatformPosix.cpp
            PosixMountProvider.cpp
            PosixResourceCounter.cpp
            PosixTimezone.cpp
            XFileUtils.cpp
            XHandle.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            PlatformDefs.h
            PlatformPosix.h
            PosixMountProvider.h
            PosixResourceCounter.h
            PosixTimezone.h
            XFileUtils.h
            XHandle.h
            XTimeUtils.h)

core_add_library(platform_posix)
