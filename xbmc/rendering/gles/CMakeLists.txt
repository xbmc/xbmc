if(OPENGLES_FOUND)
  set(SOURCES RenderSystemGLES.cpp
              ScreenshotSurfaceGLES.cpp
              ../MatrixGL.cpp
              GLESShader.cpp)

  set(HEADERS RenderSystemGLES.h
              ScreenshotSurfaceGLES.h
              ../MatrixGL.h
              GLESShader.h)

  if(ARCH MATCHES arm AND ENABLE_NEON)
    list(APPEND SOURCES ../MatrixGL.neon.cpp)
    if(NOT DEFINED NEON_FLAGS)
      set_source_files_properties(../MatrixGL.neon.cpp PROPERTIES COMPILE_OPTIONS -mfpu=neon)
    endif()
  endif()

  core_add_library(rendering_gles)
endif()
