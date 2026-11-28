set(SOURCES ThreadCpuTimes.cpp
            ThreadImplLinux.cpp)
set(HEADERS ThreadCpuTimes.h
            ThreadImplLinux.h)

core_add_library(platform_linux_threads)
