set(SOURCES Filesystem.cpp
            MessagePrinter.cpp
            PlatformPosix.cpp)

set(HEADERS PlatformPosix.h)

core_add_library(platform_posix)
