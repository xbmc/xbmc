set(SOURCES OSScreenSaverTVOS.mm
            WinEventsTVOS.mm
            WinSystemTVOS.mm
            VideoSyncTVos.cpp)
set(HEADERS OSScreenSaverTVOS.h
            WinEventsTVOS.h
            WinSystemTVOS.h
            VideoSyncTVos.h)
set(UNITY_EXCLUDES ${SOURCES})

core_add_library(windowing_tvos)
