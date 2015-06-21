set(SOURCES GUIMediaWindow.cpp
            GUIWindowDebugInfo.cpp
            GUIWindowFileManager.cpp
            GUIWindowHome.cpp
            GUIWindowLoginScreen.cpp
            GUIWindowPointer.cpp
            GUIWindowScreensaver.cpp
            GUIWindowScreensaverDim.cpp
            GUIWindowSplash.cpp
            GUIWindowStartup.cpp
            GUIWindowSystemInfo.cpp
            GUIWindowWeather.cpp)

set(HEADERS GUIMediaWindow.h
            GUIWindowDebugInfo.h
            GUIWindowFileManager.h
            GUIWindowHome.h
            GUIWindowLoginScreen.h
            GUIWindowPointer.h
            GUIWindowScreensaver.h
            GUIWindowScreensaverDim.h
            GUIWindowSplash.h
            GUIWindowStartup.h
            GUIWindowSystemInfo.h
            GUIWindowWeather.h)

core_add_library(windows)
add_dependencies(windows libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(windows ffmpeg)
endif()
