set(FBO_GL_SUPPORTED FALSE)
if(TARGET ${APP_NAME_LC}::OpenGl)
  set(FBO_GL_SUPPORTED TRUE)
elseif(TARGET ${APP_NAME_LC}::OpenGLES)
  get_target_property(FBO_GLES_DEFS ${APP_NAME_LC}::OpenGLES INTERFACE_COMPILE_DEFINITIONS)
  if(FBO_GLES_DEFS MATCHES "HAS_GLES=3")
    set(FBO_GL_SUPPORTED TRUE)
  endif()
endif()

if(FBO_GL_SUPPORTED AND TARGET ${APP_NAME_LC}::EGL)
  set(SOURCES HwRenderingContext.cpp
              HwRenderingContextEGL.cpp)
  set(HEADERS IHwRenderingContext.h
              HwRenderingContextEGL.h
              HwRenderingContextEGLUtils.h)

  core_add_library(rp-rendering-contexts)
endif()
