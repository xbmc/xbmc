if(TARGET ${APP_NAME_LC}::OpenGLES)
  set(SOURCES RenderSystemGLES.cpp
              ScreenshotSurfaceGLES.cpp
              GLESShader.cpp
              GuiCompositeShaderGLES.cpp)

  set(HEADERS RenderSystemGLES.h
              ScreenshotSurfaceGLES.h
              GLESShader.h
              GuiCompositeShaderGLES.h)

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND HEADERS RenderSystemGLESWebOS.h)
endif()

  core_add_library(rendering_gles)
endif()
