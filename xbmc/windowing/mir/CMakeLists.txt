set(SOURCES GLContextEGL.cpp
            WinEventsMir.cpp
            WinSystemMir.cpp
            WinSystemMirGLContext.cpp
            WinSystemMirGLESContext.cpp)

set(HEADERS WinSystemMir.h
            WinSystemMirGLContext.h
            WinSystemMirGLESContext.h)

core_add_library(windowing_MIR)
