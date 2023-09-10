if(TARGET OpenGL::GLES)
  set(SOURCES RenderSystemGLES.cpp
              ScreenshotSurfaceGLES.cpp
              GLESShader.cpp)

  set(HEADERS RenderSystemGLES.h
              ScreenshotSurfaceGLES.h
              GLESShader.h)

  core_add_library(rendering_gles)
endif()
