set(SOURCES Mmap.cpp
            PosixInterfaceForCLog.cpp
            SharedMemory.cpp)

set(HEADERS FileHandle.h
            Mmap.h
            PosixInterfaceForCLog.h
            SharedMemory.h)

core_add_library(platform_posix_utils)
