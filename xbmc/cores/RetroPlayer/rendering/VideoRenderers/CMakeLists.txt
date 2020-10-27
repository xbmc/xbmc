set(SOURCES RPBaseRenderer.cpp
            RPRendererGuiTexture.cpp)
set(HEADERS RPBaseRenderer.h
            RPRendererGuiTexture.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES RPWinRenderer.cpp)
  list(APPEND HEADERS RPWinRenderer.h)
endif()

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES RPRendererOpenGLES.cpp)
  list(APPEND HEADERS RPRendererOpenGLES.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES RPRendererOpenGL.cpp)
  list(APPEND HEADERS RPRendererOpenGL.h)
endif()

if(("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC) AND EGL_FOUND)
  list(APPEND SOURCES RPRendererDMA.cpp)
  list(APPEND HEADERS RPRendererDMA.h)
endif()

core_add_library(rp-videorenderers)
