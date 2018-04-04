set(SOURCES APKDirectory.cpp
            APKFile.cpp
            AndroidAppDirectory.cpp
            AndroidAppFile.cpp)
set(HEADERS APKDirectory.h
            APKFile.h
            AndroidAppDirectory.h
            AndroidAppFile.h)

core_add_library(platform_android_filesystem)
