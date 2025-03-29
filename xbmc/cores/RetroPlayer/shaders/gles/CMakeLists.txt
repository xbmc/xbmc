if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES ShaderGLES.cpp
                      ShaderLutGLES.cpp
                      ShaderPresetGLES.cpp
                      ShaderTextureGLES.cpp
                      ShaderUtilsGLES.cpp
  )
  list(APPEND HEADERS ShaderGLES.h
                      ShaderLutGLES.h
                      ShaderPresetGLES.h
                      ShaderTextureGLES.h
                      ShaderUtilsGLES.cpp
  )

  core_add_library(rp-shaders-gles)
endif()
