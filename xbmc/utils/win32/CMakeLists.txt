set(SOURCES Win32InterfaceForCLog.cpp
            Win32Log.cpp)

set(HEADERS gpu_memcpy_sse4.h
            memcpy_sse2.h
            Win32InterfaceForCLog.h
            Win32Log.h)

core_add_library(utils_win32)
