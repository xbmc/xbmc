set(SOURCES BaseRenderBuffer.cpp
            BaseRenderBufferPool.cpp
            RenderBufferManager.cpp
)

set(HEADERS BaseRenderBuffer.h
            BaseRenderBufferPool.h
            IRenderBuffer.h
            IRenderBufferPool.h
            RenderBufferManager.h
)

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES RenderBufferOpenGLES.cpp
                      RenderBufferPoolOpenGLES.cpp)
  list(APPEND HEADERS RenderBufferOpenGLES.h
                      RenderBufferPoolOpenGLES.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES RenderBufferOpenGL.cpp
                      RenderBufferPoolOpenGL.cpp)
  list(APPEND HEADERS RenderBufferOpenGL.cpp
                      RenderBufferPoolOpenGL.cpp)
endif()

if(GBM_HAS_BO_MAP AND OPENGLES_FOUND)
  list(APPEND SOURCES RenderBufferGBM.cpp
                      RenderBufferPoolGBM.cpp)
  list(APPEND HEADERS RenderBufferGBM.h
                      RenderBufferPoolGBM.h)
endif()

core_add_library(rp-buffers)
