set(SOURCES LinuxStorageProvider.cpp)

set(HEADERS LinuxStorageProvider.h)

if(TARGET ${APP_NAME_LC}::DBus)
  list(APPEND SOURCES UDisksProvider.cpp
                      UDisks2Provider.cpp)
  list(APPEND HEADERS UDisksProvider.h
                      UDisks2Provider.h)
endif()

if(TARGET ${APP_NAME_LC}::UDEV)
  list(APPEND SOURCES UDevProvider.cpp)
  list(APPEND HEADERS UDevProvider.h)
endif()

if(SOURCES)
  core_add_library(platform_linux_storage)
endif()
