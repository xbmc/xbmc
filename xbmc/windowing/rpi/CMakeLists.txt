set(SOURCES WinSystemRpi.cpp
            RPIUtils.cpp
            VideoSyncPi.cpp)

set(HEADERS WinSystemRpi.h
            RPIUtils.h
            VideoSyncPi.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemRpiGLESContext.cpp)
  list(APPEND HEADERS WinSystemRpiGLESContext.h)
endif()

core_add_library(windowing_Rpi)
