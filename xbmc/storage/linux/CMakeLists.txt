set(SOURCES DeviceKitDisksProvider.cpp
            UDisksProvider.cpp)

set(HEADERS DeviceKitDisksProvider.h
            LinuxStorageProvider.h
            UDisksProvider.h)

if(UDEV_FOUND)
  list(APPEND SOURCES UDevProvider.cpp)
  list(APPEND HEADERS UDevProvider.h)
endif()

core_add_library(storage_linux)
