list(APPEND PLATFORM_REQUIRED_DEPS EGL X XRandR LibDRM)
list(APPEND PLATFORM_OPTIONAL_DEPS VAAPI)

if(APP_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
  list(APPEND PLATFORM_OPTIONAL_DEPS GLX VDPAU)
elseif(APP_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES)
endif()
