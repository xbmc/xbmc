set(SOURCES android_main.cpp
            AndroidFeatures.cpp
            AndroidJoyStick.cpp
            AndroidKey.cpp
            AndroidMouse.cpp
            AndroidTouch.cpp
            EventLoop.cpp
            GraphicBuffer.cpp
            JNIMainActivity.cpp
            JNIXBMCVideoView.cpp
            XBMCApp.cpp
            ${NDKROOT}/sources/android/native_app_glue/android_native_app_glue.c
            ${NDKROOT}/sources/android/cpufeatures/cpu-features.c)

set(HEADERS AndroidExtra.h
            AndroidFeatures.h
            AndroidJoyStick.h
            AndroidKey.h
            AndroidMouse.h
            AndroidTouch.h
            DllGraphicBuffer.h
            EventLoop.h
            GraphicBuffer.h
            IActivityHandler.h
            JNIMainActivity.h
            JNIXBMCVideoView.h
            XBMCApp.h)

core_add_library(platform_android_activity)
target_include_directories(${CORE_LIBRARY}
                           PRIVATE ${NDKROOT}/sources/android/native_app_glue
                                   ${NDKROOT}/sources/android/cpufeatures)
