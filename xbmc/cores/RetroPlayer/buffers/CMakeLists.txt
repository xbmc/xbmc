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

if(("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC) AND EGL_FOUND)
  list(APPEND SOURCES RenderBufferDMA.cpp
                      RenderBufferPoolDMA.cpp)
  list(APPEND HEADERS RenderBufferDMA.h
                      RenderBufferPoolDMA.h)
endif()

core_add_library(rp-buffers)
