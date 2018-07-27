set(PLATFORM_REQUIRED_DEPS EGL GBM LibDRM LibInput Xkbcommon)
set(PLATFORM_OPTIONAL_DEPS VAAPI)

set(GBM_RENDER_SYSTEM "" CACHE STRING "Render system to use with GBM: \"gl\" or \"gles\"")

if(GBM_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
  set(APP_RENDER_SYSTEM gl)
elseif(GBM_RENDER_SYSTEM STREQUAL "gles")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGLES)
  set(APP_RENDER_SYSTEM gles)
else()
  message(SEND_ERROR "You need to decide whether you want to use GL- or GLES-based rendering in combination with the GBM windowing system. Please set GBM_RENDER_SYSTEM to either \"gl\" or \"gles\". For normal desktop systems, you will usually want to use \"gl\".")
endif()

# __GBM__ is needed by eglplatform.h in case it is included before gbm.h
list(APPEND PLATFORM_DEFINES -DMESA_EGL_NO_X11_HEADERS -D__GBM__=1 -DPLATFORM_SETTINGS_FILE=gbm.xml)
