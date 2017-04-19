set(SOURCES GLContextEGL.cpp
            WinEventsAndroid.cpp
            WinSystemAndroid.cpp
            AndroidUtils.cpp
            VideoSyncAndroid.cpp)

set(HEADERS GLContextEGL.h
            WinEventsAndroid.h
            WinSystemAndroid.h
            AndroidUtils.h
            VideoSyncAndroid.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemAndroidGLESContext.cpp)
  list(APPEND HEADERS WinSystemAndroidGLESContext.h)
endif()

core_add_library(windowing_android)
