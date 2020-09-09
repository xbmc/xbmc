set(SOURCES CocoaDPMSSupport.cpp
            OSScreenSaverOSX.cpp
            WinEventsOSX.mm
            WinEventsSDL.cpp
            WinSystemOSX.mm
            WinSystemOSXGL.mm
            VideoSyncOsx.mm)
set(HEADERS CocoaDPMSSupport.h
            OSScreenSaverOSX.h
            WinEventsOSX.h
            WinEventsSDL.h
            WinSystemOSX.h
            WinSystemOSXGL.h
            VideoSyncOsx.h)

set(UNITY_EXCLUDES VideoSyncOsx.mm
                   WinSystemOSX.mm
                   WinSystemOSXGL.mm)

core_add_library(windowing_osx)
