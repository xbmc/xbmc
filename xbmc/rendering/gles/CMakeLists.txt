if(OPENGLES_FOUND)
  set(SOURCES RenderSystemGLES.cpp
              ../MatrixGL.cpp
              GLESShader.cpp)

  set(HEADERS RenderSystemGLES.h
              ../MatrixGL.h
              GLESShader.h)

  core_add_library(rendering_gles)
endif()
