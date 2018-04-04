set(SOURCES Win32InterfaceForCLog.cpp)

set(HEADERS gpu_memcpy_sse4.h
            memcpy_sse2.h
            Win32InterfaceForCLog.h)

core_add_library(platform_win32_utils)
