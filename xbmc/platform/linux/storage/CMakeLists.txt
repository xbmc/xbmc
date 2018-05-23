set(SOURCES LinuxStorageProvider.cpp)

set(HEADERS LinuxStorageProvider.h)

if(DBUS_FOUND)
  list(APPEND SOURCES UDisksProvider.cpp
                      UDisks2Provider.cpp)
  list(APPEND HEADERS UDisksProvider.h
                      UDisks2Provider.h)
endif()

if(UDEV_FOUND)
  list(APPEND SOURCES UDevProvider.cpp)
  list(APPEND HEADERS UDevProvider.h)
endif()

if(SOURCES)
  core_add_library(platform_linux_storage)
endif()
