set(SOURCES ConvUtils.cpp
            Filesystem.cpp
            LinuxResourceCounter.cpp
            LinuxTimezone.cpp
            MessagePrinter.cpp
            PlatformPosix.cpp
            XFileUtils.cpp
            XHandle.cpp
            XMemUtils.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            LinuxResourceCounter.h
            LinuxTimezone.h
            PlatformPosix.h
            XFileUtils.h
            XHandle.h
            XMemUtils.h
            XTimeUtils.h)

core_add_library(platform_posix)
