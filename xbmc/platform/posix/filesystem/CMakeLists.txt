set(SOURCES PosixDirectory.cpp
            PosixFile.cpp)

set(HEADERS PosixDirectory.h
            PosixFile.h)

if(SMBCLIENT_FOUND)
  list(APPEND SOURCES SMBDirectory.cpp
                      SMBFile.cpp)
  list(APPEND HEADERS SMBDirectory.h
                      SMBFile.h)
endif()

core_add_library(platform_posix_filesystem)
