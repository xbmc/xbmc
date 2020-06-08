set(SOURCES AddonBase.cpp
            AudioEngine.cpp
            General.cpp
            Filesystem.cpp
            Network.cpp)

set(HEADERS AddonBase.h
            AudioEngine.h
            General.h
            Filesystem.h
            Network.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES platform/android/System.cpp)
  list(APPEND HEADERS platform/android/System.h)
endif()

core_add_library(addons_interfaces)
