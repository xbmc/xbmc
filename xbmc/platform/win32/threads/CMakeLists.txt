set(SOURCES ThreadImplWin.cpp
            Win32Exception.cpp)

set(HEADERS ThreadImplWin.h
            Win32Exception.h)

core_add_library(threads_win)
