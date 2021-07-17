set(SOURCES PosixDirectory.cpp
            PosixFile.cpp)

set(HEADERS PosixDirectory.h
            PosixFile.h)

if(SMBCLIENT_FOUND)
  list(APPEND SOURCES SMBDirectory.cpp
                      SMBFile.cpp
                      SMBWSDiscovery.cpp
                      SMBWSDiscoveryListener.cpp)
  list(APPEND HEADERS SMBDirectory.h
                      SMBFile.h
                      SMBWSDiscovery.h
                      SMBWSDiscoveryListener.h)
endif()

core_add_library(platform_posix_filesystem)
