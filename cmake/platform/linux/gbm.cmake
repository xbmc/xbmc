list(APPEND PLATFORM_REQUIRED_DEPS EGL GBM LibDRM LibInput Xkbcommon UDEV)
list(APPEND PLATFORM_OPTIONAL_DEPS VAAPI)

if(APP_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
elseif(APP_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES)
endif()
