if(TARGET ${APP_NAME_LC}::OpenGl)
  set(SOURCES TestShaderUtilsGL.cpp)

  core_add_test_library(retroplayer_shaders_gl_test)
endif()
