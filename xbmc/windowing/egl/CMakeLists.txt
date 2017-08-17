set(SOURCES EGLNativeTypeIMX.cpp
            EGLWrapper.cpp
            WinSystemEGL.cpp)

set(HEADERS EGLNativeType.h
            EGLNativeTypeIMX.h
            EGLQuirks.h
            EGLWrapper.h
            WinSystemEGL.h)

core_add_library(windowing_egl)
