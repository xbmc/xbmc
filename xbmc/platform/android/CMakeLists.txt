set(SOURCES CPUInfoAndroid.cpp
            GPUInfoAndroid.cpp
            MemUtils.cpp
            PlatformAndroid.cpp
            ${NDKROOT}/sources/android/cpufeatures/cpu-features.c)

set(HEADERS CPUInfoAndroid.h
            GPUInfoAndroid.h
            PlatformAndroid.h)

set_source_files_properties(${NDKROOT}/sources/android/cpufeatures/cpu-features.c PROPERTIES
                            SKIP_LINTING ON)
core_add_library(androidsupport)
target_include_directories(${CORE_LIBRARY} SYSTEM
                           PRIVATE ${NDKROOT}/sources/android/cpufeatures)
