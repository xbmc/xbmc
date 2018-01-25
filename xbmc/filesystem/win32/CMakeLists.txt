set(SOURCES Win32Directory.cpp
            Win32File.cpp
            Win32SMBFile.cpp)

set(HEADERS Win32Directory.h
            Win32File.h
            Win32SMBFile.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES Win32SMBDirectory.cpp)
  list(APPEND HEADERS Win32SMBDirectory.h)
endif()

core_add_library(filesystem_win32)
