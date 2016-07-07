set(SOURCES coff.cpp
            dll.cpp
            DllLoader.cpp
            DllLoaderContainer.cpp
            dll_tracker.cpp
            dll_tracker_file.cpp
            dll_tracker_library.cpp
            dll_util.cpp
            LibraryLoader.cpp)

set(HEADERS coff.h
            coffldr.h
            dll.h
            DllLoader.h
            DllLoaderContainer.h
            dll_tracker.h
            dll_tracker_file.h
            dll_tracker_library.h
            dll_util.h
            LibraryLoader.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES mmap_anon.c
                      SoLoader.cpp)
  list(APPEND HEADERS mmap_anon.h
                      SoLoader.h)
  if(NOT CORE_SYSTEM_NAME STREQUAL freebsd)
    list(APPEND SOURCES ldt_keeper.c) 
    list(APPEND HEADERS ldt_keeper.h)
  endif()
else()
  list(APPEND SOURCES Win32DllLoader.cpp)
  list(APPEND HEADERS Win32DllLoader.h)
endif()

add_definitions(-DAPI_DEBUG)

core_add_library(dllloader)
