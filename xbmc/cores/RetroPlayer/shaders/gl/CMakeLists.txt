if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES ShaderGL.cpp
                      ShaderLutGL.cpp
                      ShaderPresetGL.cpp
                      ShaderTextureGL.cpp
                      ShaderUtilsGL.cpp
  )
  list(APPEND HEADERS ShaderGL.h
                      ShaderLutGL.h
                      ShaderPresetGL.h
                      ShaderTextureGL.h
                      ShaderUtilsGL.h
  )

  core_add_library(rp-shaders-gl)
endif()
