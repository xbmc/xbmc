if(TARGET ${APP_NAME_LC}::OpenGLES)
  set(SOURCES TestShaderUtilsGLES.cpp)

  core_add_test_library(retroplayer_shaders_gles_test)
endif()
