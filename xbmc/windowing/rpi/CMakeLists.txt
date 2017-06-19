set(SOURCES GLContextEGL.cpp
            WinSystemRpi.cpp
            RPIUtils.cpp
            VideoSyncPi.cpp)

set(HEADERS GLContextEGL.h
            WinSystemRpi.h
            RPIUtils.h
            VideoSyncPi.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemRpiGLESContext.cpp)
  list(APPEND HEADERS WinSystemRpiGLESContext.h)
endif()

core_add_library(windowing_Rpi)
