set(PLATFORM_REQUIRED_DEPS OpenGLES EGL GBM LibDRM LibInput Xkbcommon)
set(PLATFORM_OPTIONAL_DEPS VAAPI)
set(APP_RENDER_SYSTEM gles)
# __GBM__ is needed by eglplatform.h in case it is included before gbm.h
list(APPEND PLATFORM_DEFINES -DMESA_EGL_NO_X11_HEADERS -D__GBM__=1 -DPLATFORM_SETTINGS_FILE=gbm.xml)
