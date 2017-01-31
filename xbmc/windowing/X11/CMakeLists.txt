set(SOURCES GLContextEGL.cpp
            GLContextGLX.cpp
            GLContext.cpp
            WinSystemX11GLContext.cpp
            WinSystemX11GLESContext.cpp
            XRandR.cpp
            VideoSyncGLX.cpp
            VideoSyncDRM.cpp)

set(HEADERS GLContext.h
            GLContextEGL.h
            GLContextGLX.h
            WinSystemX11GLContext.h
            WinSystemX11GLESContext.h
            XRandR.h
            VideoSyncGLX.h
            VideoSyncDRM.h)

if(NOT CORE_SYSTEM_NAME STREQUAL rbpi)
  list(APPEND SOURCES WinSystemX11.cpp)
  list(APPEND HEADERS WinSystemX11.h)
endif()

core_add_library(windowing_X11)
