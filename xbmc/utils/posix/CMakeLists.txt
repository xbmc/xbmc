set(SOURCES Mmap.cpp
            PosixInterfaceForCLog.cpp)

set(HEADERS FileHandle.h
            Mmap.h
            PosixInterfaceForCLog.h)

core_add_library(utils_posix)
