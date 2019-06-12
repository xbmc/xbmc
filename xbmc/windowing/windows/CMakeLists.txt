set(SOURCES VideoSyncD3D.cpp
            Win32DPMSSupport.cpp
            WinEventsWin32.cpp
            WinSystemWin32.cpp
            WinSystemWin32DX.cpp)

set(HEADERS VideoSyncD3D.h
            Win32DPMSSupport.h
            WinEventsWin32.h
            WinSystemWin32.h
            WinSystemWin32DX.h
            WinKeyMap.h)

core_add_library(windowing_windows)
