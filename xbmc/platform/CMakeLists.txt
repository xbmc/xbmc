set(SOURCES Environment.cpp
            xbmc.cpp)

set(HEADERS Environment.h
            Filesystem.h
            MessagePrinter.h
            Platform.h
            xbmc.h)

core_add_library(platform_common)
