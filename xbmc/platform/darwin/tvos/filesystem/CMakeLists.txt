set(SOURCES TVOSDirectory.cpp
            TVOSFile.cpp
            TVOSFileUtils.mm)

set(HEADERS TVOSDirectory.h
            TVOSFile.h
            TVOSFileUtils.h)

core_add_library(platform_tvos_filesystem)
