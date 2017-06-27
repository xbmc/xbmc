set(SOURCES "")

set(HEADERS LinuxStorageProvider.h)

if(DBUS_FOUND)
  list(APPEND SOURCES UDisksProvider.cpp)
  list(APPEND HEADERS UDisksProvider.h)
endif()

if(UDEV_FOUND)
  list(APPEND SOURCES UDevProvider.cpp)
  list(APPEND HEADERS UDevProvider.h)
endif()

if(SOURCES)
  core_add_library(storage_linux)
endif()
