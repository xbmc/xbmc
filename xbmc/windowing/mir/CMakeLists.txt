set(SOURCES GLContextEGL.cpp
            WinEventsMir.cpp
            WinSystemMir.cpp)

set(HEADERS WinSystemMir.h)

if(OPENGL_FOUND)
  list(APPEND SOURCES WinSystemMirGLContext.cpp)
  list(APPEND HEADERS WinSystemMirGLContext.h)
elseif(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemMirGLESContext.cpp)
  list(APPEND HEADERS WinSystemMirGLESContext.h)
endif()

core_add_library(windowing_MIR)
