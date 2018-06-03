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

core_add_library(rp-buffers)
