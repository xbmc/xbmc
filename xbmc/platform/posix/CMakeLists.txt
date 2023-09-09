set(SOURCES ConvUtils.cpp
            CPUInfoPosix.cpp
            Filesystem.cpp
            GPUInfoPosix.cpp
            MessagePrinter.cpp
            PlatformPosix.cpp
            PosixMountProvider.cpp
            PosixResourceCounter.cpp
            PosixTimezone.cpp
            XHandle.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            CPUInfoPosix.h
            GPUInfoPosix.h
            PlatformDefs.h
            PlatformPosix.h
            PosixMountProvider.h
            PosixResourceCounter.h
            PosixTimezone.h
            XHandle.h)

core_add_library(platform_posix)
