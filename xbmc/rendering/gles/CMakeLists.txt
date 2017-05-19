if(OPENGLES_FOUND)
  set(SOURCES RenderSystemGLES.cpp)

  set(HEADERS RenderSystemGLES.h)

  core_add_library(rendering_gles)
endif()
