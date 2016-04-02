set(SOURCES GLContextEGL.cpp
            GLContextGLX.cpp
            GLContext.cpp
            WinSystemX11.cpp
            WinSystemX11GLContext.cpp
            WinSystemX11GLESContext.cpp
            XRandR.cpp)

set(HEADERS GLContext.h
            GLContextEGL.h
            GLContextGLX.h
            WinSystemX11.h
            WinSystemX11GLContext.h
            WinSystemX11GLESContext.h
            XRandR.h)

core_add_library(windowing_X11)
