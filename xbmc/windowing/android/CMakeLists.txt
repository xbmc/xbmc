set(SOURCES OSScreenSaverAndroid.cpp
            WinEventsAndroid.cpp
            WinSystemAndroid.cpp
            AndroidUtils.cpp
            VideoSyncAndroid.cpp)

set(HEADERS OSScreenSaverAndroid.h
            WinEventsAndroid.h
            WinSystemAndroid.h
            AndroidUtils.h
            VideoSyncAndroid.h)

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES WinSystemAndroidGLESContext.cpp)
  list(APPEND HEADERS WinSystemAndroidGLESContext.h)
endif()

core_add_library(windowing_android)
