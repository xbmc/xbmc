set(PLATFORM_REQUIRED_DEPS LibAndroidJNI OpenGLES EGL Zip)
set(APP_RENDER_SYSTEM gles)

# Store SDK compile version
set(TARGET_SDK 29)
# Minimum supported SDK version
set(TARGET_MINSDK 21)

if(DEFINED ENV{ANDROIDSTORE} AND ("$ENV{ANDROIDSTORE}" STREQUAL "GOOGLE"))
  set(PLAYSTORE_STATE "true")
else()
  set(PLAYSTORE_STATE "false")
endif()
