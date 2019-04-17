set(SOURCES OSScreenSaverOSX.cpp
            WinEventsOSX.mm
            WinEventsSDL.cpp
            WinSystemOSX.mm
            WinSystemOSXGL.mm
            VideoSyncOsx.cpp)
set(HEADERS OSScreenSaverOSX.h
            WinEventsOSX.h
            WinEventsSDL.h
            WinSystemOSX.h
            WinSystemOSXGL.h
            VideoSyncOsx.h)

core_add_library(windowing_osx)
