list(APPEND PLATFORM_REQUIRED_DEPS GBM LibDRM>=2.4.95 LibInput Xkbcommon UDEV LibDisplayInfo)
list(APPEND PLATFORM_OPTIONAL_DEPS VAAPI)

if(APP_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl EGL)
elseif(APP_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES EGL)
endif()
