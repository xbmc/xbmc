set(SOURCES Win32Directory.cpp
            Win32File.cpp
            Win32SMBDirectory.cpp
            Win32SMBFile.cpp)

set(HEADERS Win32Directory.h
            Win32File.h
            Win32SMBDirectory.h
            Win32SMBFile.h)

core_add_library(filesystem_win32)
