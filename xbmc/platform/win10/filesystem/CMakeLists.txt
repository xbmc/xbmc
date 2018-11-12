set(SOURCES UwpSMBDirectory.cpp
            UwpSMBFile.cpp
            WinLibraryDirectory.cpp
            WinLibraryFile.cpp)

set(HEADERS UwpSMBDirectory.h
            UwpSMBFile.h
            WinLibraryDirectory.h
            WinLibraryFile.h)

core_add_library(platform_win10_filesystem)
