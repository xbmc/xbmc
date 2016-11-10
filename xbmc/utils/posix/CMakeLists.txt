set(SOURCES Mmap.cpp
            SharedMemory.cpp)

set(HEADERS FileHandle.h
            Mmap.h
            SharedMemory.h)

core_add_library(utils_posix)
