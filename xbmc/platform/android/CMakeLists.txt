set(SOURCES CPUInfoAndroid.cpp
            GPUInfoAndroid.cpp
            MemUtils.cpp
            PlatformAndroid.cpp
            ${NDKROOT}/sources/android/cpufeatures/cpu-features.c)

set(HEADERS CPUInfoAndroid.h
            GPUInfoAndroid.h
            PlatformAndroid.h)

add_subdirectory(utils)
add_subdirectory(activity)
add_subdirectory(filesystem)
add_subdirectory(media)
add_subdirectory(network)
add_subdirectory(powermanagement)
add_subdirectory(speech)
add_subdirectory(storage)

core_add_library(androidsupport)
target_include_directories(${CORE_LIBRARY} SYSTEM
                           PRIVATE ${NDKROOT}/sources/android/cpufeatures)
target_link_libraries(${CORE_LIBRARY} PRIVATE android_utils)
