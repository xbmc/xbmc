set(SOURCES dll.cpp
            DllLoaderContainer.cpp
            LibraryLoader.cpp)

set(HEADERS dll.h
            DllLoaderContainer.h
            LibraryLoader.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES SoLoader.cpp)
  list(APPEND HEADERS SoLoader.h)
else()
  list(APPEND SOURCES Win32DllLoader.cpp)
  list(APPEND HEADERS Win32DllLoader.h)
endif()

add_definitions(-DAPI_DEBUG)

core_add_library(dllloader)
