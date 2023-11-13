set(SOURCES GLContextEGL.cpp
            GLContext.cpp
            OptionalsReg.cpp
            OSScreenSaverX11.cpp
            WinEventsX11.cpp
            WinSystemX11.cpp
            XRandR.cpp
            X11DPMSSupport.cpp)

set(HEADERS GLContext.h
            GLContextEGL.h
            OptionalsReg.h
            OSScreenSaverX11.h
            WinEventsX11.h
            WinSystemX11.h
            XRandR.h
            X11DPMSSupport.h)

if(GLX_FOUND)
  list(APPEND SOURCES GLContextGLX.cpp
                      VideoSyncGLX.cpp)
  list(APPEND HEADERS GLContextGLX.h
                      VideoSyncGLX.h)
endif()

if(TARGET OpenGL::GL)
  list(APPEND SOURCES WinSystemX11GLContext.cpp)
  list(APPEND HEADERS WinSystemX11GLContext.h)
  list(APPEND SOURCES VideoSyncOML.cpp)
  list(APPEND HEADERS VideoSyncOML.h)
endif()
if(TARGET OpenGL::GLES)
  list(APPEND SOURCES WinSystemX11GLESContext.cpp)
  list(APPEND HEADERS WinSystemX11GLESContext.h)
endif()

core_add_library(windowing_X11)
