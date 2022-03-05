set(SOURCES RecursiveMutex.cpp
            ThreadImplPosix.cpp)
set(HEADERS RecursiveMutex.h
            ThreadImplPosix.h)

core_add_library(platform_posix_threads)
