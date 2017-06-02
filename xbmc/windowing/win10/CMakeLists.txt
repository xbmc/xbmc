set(SOURCES WinEventsWin10.cpp
            WinSystemWin10.cpp
            WinSystemWin10DX.cpp
            ../windows/VideoSyncD3D.cpp)

set(HEADERS WinEventsWin10.h
            WinSystemWin10.h
            WinSystemWin10DX.h
            ../windows/VideoSyncD3D.h
            ../windows/WinKeyMap.h)

core_add_library(windowing_windowsstore)
