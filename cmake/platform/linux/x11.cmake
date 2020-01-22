set(PLATFORM_REQUIRED_DEPS EGL X XRandR LibDRM)
set(PLATFORM_OPTIONAL_DEPS VAAPI)

set(X11_RENDER_SYSTEM "" CACHE STRING "Render system to use with X11: \"gl\" or \"gles\"")

if(X11_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
  list(APPEND PLATFORM_OPTIONAL_DEPS GLX VDPAU)
  set(APP_RENDER_SYSTEM gl)
elseif(X11_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES)
  set(APP_RENDER_SYSTEM gles)
else()
  message(SEND_ERROR "You need to decide whether you want to use GL- or GLES-based rendering in combination with the X11 windowing system. Please set X11_RENDER_SYSTEM to either \"gl\" or \"gles\". For normal desktop systems, you will usually want to use \"gl\".")
endif()

list(APPEND PLATFORM_DEFINES -DPLATFORM_SETTINGS_FILE=x11.xml)