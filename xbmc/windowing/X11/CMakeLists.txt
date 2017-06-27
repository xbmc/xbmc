set(SOURCES GLContextEGL.cpp
            GLContextGLX.cpp
            GLContext.cpp
            OSScreenSaverX11.cpp
            WinEventsX11.cpp
            WinSystemX11.cpp
            WinSystemX11GLContext.cpp
            XRandR.cpp
            VideoSyncGLX.cpp
            VideoSyncDRM.cpp)

set(HEADERS GLContext.h
            GLContextEGL.h
            GLContextGLX.h
            OSScreenSaverX11.h
            WinEventsX11.h
            WinSystemX11.h
            WinSystemX11GLContext.h
            XRandR.h
            VideoSyncGLX.h
            VideoSyncDRM.h)

core_add_library(windowing_X11)
