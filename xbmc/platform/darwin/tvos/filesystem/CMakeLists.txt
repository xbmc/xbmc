set(SOURCES TVOSDirectory.cpp
            TVOSFile.cpp
            TVOSFileUtils.mm)

set(HEADERS TVOSDirectory.h
            TVOSFile.h
            TVOSFileUtils.h)

set(UNITY_EXCLUDES TVOSFileUtils.mm)

core_add_library(platform_tvos_filesystem)
