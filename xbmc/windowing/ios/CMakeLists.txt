set(SOURCES WinEventsIOS.mm
            WinSystemIOS.mm
            VideoSyncIos.cpp)
set(HEADERS WinEventsIOS.h
            WinSystemIOS.h
            VideoSyncIos.h)

set(UNITY_EXCLUDES ${SOURCES})

core_add_library(windowing_ios)
