set(SOURCES LinuxStorageProvider.cpp)

set(HEADERS LinuxStorageProvider.h)

if(TARGET DBus::DBus)
  list(APPEND SOURCES UDisksProvider.cpp
                      UDisks2Provider.cpp)
  list(APPEND HEADERS UDisksProvider.h
                      UDisks2Provider.h)
endif()

if(TARGET UDEV::UDEV)
  list(APPEND SOURCES UDevProvider.cpp)
  list(APPEND HEADERS UDevProvider.h)
endif()

if(SOURCES)
  core_add_library(platform_linux_storage)
endif()
