set(SOURCES RPBaseRenderer.cpp
            RPRendererGuiTexture.cpp)
set(HEADERS RenderGeometryFBO.h
            RPBaseRenderer.h
            RPRendererGuiTexture.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES RPWinRenderer.cpp)
  list(APPEND HEADERS RPWinRenderer.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES RPRendererOpenGL.cpp)
  list(APPEND HEADERS RPRendererOpenGL.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES RPRendererOpenGLES.cpp)
  list(APPEND HEADERS RPRendererOpenGLES.h)
endif()

if(("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC) AND TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES RPRendererDMAUtils.cpp)
  list(APPEND HEADERS RPRendererDMAUtils.h)

  if(TARGET ${APP_NAME_LC}::OpenGl)
    list(APPEND SOURCES RPRendererDMAOpenGL.cpp)
    list(APPEND HEADERS RPRendererDMAOpenGL.h)
  endif()

  if(TARGET ${APP_NAME_LC}::OpenGLES)
    list(APPEND SOURCES RPRendererDMAOpenGLES.cpp)
    list(APPEND HEADERS RPRendererDMAOpenGLES.h)
  endif()
endif()

# GLES2 builds only the factory, which declines hardware rendering.
if(TARGET ${APP_NAME_LC}::EGL OR CORE_SYSTEM_NAME STREQUAL "osx")
  list(APPEND SOURCES RPRendererFBO.cpp)
  list(APPEND HEADERS RPRendererFBO.h)
endif()

core_add_library(rp-videorenderers)
