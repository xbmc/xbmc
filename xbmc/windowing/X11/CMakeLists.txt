set(SOURCES GLContextEGL.cpp
            GLContext.cpp
            OptionalsReg.cpp
            OSScreenSaverX11.cpp
            WinEventsX11.cpp
            WinSystemX11.cpp
            WinSystemX11GLContext.cpp
            XRandR.cpp
            VideoSyncOML.cpp)

set(HEADERS GLContext.h
            GLContextEGL.h
            OptionalsReg.h
            OSScreenSaverX11.h
            WinEventsX11.h
            WinSystemX11.h
            WinSystemX11GLContext.h
            XRandR.h
            VideoSyncOML.h)

if(GLX_FOUND)
  list(APPEND SOURCES GLContextGLX.cpp
                      VideoSyncGLX.cpp)
  list(APPEND HEADERS GLContextGLX.h
                      VideoSyncGLX.h)
endif()

core_add_library(windowing_X11)
