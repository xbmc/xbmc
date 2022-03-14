set(SOURCES android_main.cpp
            AndroidFeatures.cpp
            AndroidJoyStick.cpp
            AndroidKey.cpp
            AndroidMouse.cpp
            AndroidTouch.cpp
            EventLoop.cpp
            GraphicBuffer.cpp
            JNIMainActivity.cpp
            JNIXBMCMainView.cpp
            JNIXBMCMediaSession.cpp
            JNIXBMCVideoView.cpp
            JNIXBMCAudioManagerOnAudioFocusChangeListener.cpp
            JNIXBMCSurfaceTextureOnFrameAvailableListener.cpp
            JNIXBMCNsdManagerDiscoveryListener.cpp
            JNIXBMCNsdManagerRegistrationListener.cpp
            JNIXBMCNsdManagerResolveListener.cpp
            JNIXBMCJsonHandler.cpp
            JNIXBMCFile.cpp
            JNIXBMCURIUtils.cpp
            JNIXBMCDisplayManagerDisplayListener.cpp
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
            JNIXBMCMainView.h
            JNIXBMCMediaSession.h
            JNIXBMCVideoView.h
            JNIXBMCAudioManagerOnAudioFocusChangeListener.h
            JNIXBMCSurfaceTextureOnFrameAvailableListener.h
            JNIXBMCNsdManagerDiscoveryListener.h
            JNIXBMCNsdManagerRegistrationListener.h
            JNIXBMCNsdManagerResolveListener.h
            JNIXBMCJsonHandler.h
            JNIXBMCFile.h
            JNIXBMCURIUtils.h
            JNIXBMCDisplayManagerDisplayListener.h
            XBMCApp.h)

core_add_library(platform_android_activity)
target_include_directories(${CORE_LIBRARY} SYSTEM
                           PRIVATE ${NDKROOT}/sources/android/native_app_glue
                                   ${NDKROOT}/sources/android/cpufeatures)
