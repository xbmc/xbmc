set(SOURCES WinEventsAndroid.cpp
            WinSystemAndroid.cpp
            AndroidUtils.cpp
            VideoSyncAndroid.cpp)

set(HEADERS WinEventsAndroid.h
            WinSystemAndroid.h
            AndroidUtils.h
            VideoSyncAndroid.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemAndroidGLESContext.cpp)
  list(APPEND HEADERS WinSystemAndroidGLESContext.h)
endif()

core_add_library(windowing_android)
