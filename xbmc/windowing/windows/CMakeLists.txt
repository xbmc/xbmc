set(SOURCES VideoSyncD3D.cpp
            WinEventsWin32.cpp
            WinSystemWin32.cpp
            WinSystemWin32DX.cpp)

set(HEADERS VideoSyncD3D.h
            WinEventsWin32.h
            WinSystemWin32.h
            WinSystemWin32DX.h)

core_add_library(windowing_windows)
