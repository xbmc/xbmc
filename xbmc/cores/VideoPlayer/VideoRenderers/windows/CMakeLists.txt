set(SOURCES RendererBase.cpp
            RendererDXVA.cpp
            RendererHQ.cpp
            RendererShaders.cpp
            RendererSoftware.cpp
            RendererPL.cpp)

set(HEADERS RendererBase.h
            RendererDXVA.h
            RendererHQ.h
            RendererShaders.h
            RendererSoftware.h            
            RendererPL.h)
core_add_library(videorenderers-windows)
