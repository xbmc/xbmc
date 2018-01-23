if(OPENGLES_FOUND)
  set(SOURCES RenderSystemGLES.cpp
              GLESShader.cpp)

  set(HEADERS RenderSystemGLES.h
              GLESShader.h)

  core_add_library(rendering_gles)
endif()
