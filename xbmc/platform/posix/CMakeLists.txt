set(SOURCES ConvUtils.cpp
            Filesystem.cpp
            LinuxTimezone.cpp
            MessagePrinter.cpp
            PlatformPosix.cpp
            PosixMountProvider.cpp
            PosixResourceCounter.cpp
            XFileUtils.cpp
            XHandle.cpp
            XMemUtils.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            LinuxTimezone.h
            PlatformPosix.h
            PosixMountProvider.h
            PosixResourceCounter.h
            XFileUtils.h
            XHandle.h
            XMemUtils.h
            XTimeUtils.h)

core_add_library(platform_posix)
