set(SOURCES RendererBase.cpp
            RendererDXVA.cpp
            RendererHQ.cpp
            RendererShaders.cpp
            RendererSoftware.cpp)

set(HEADERS RendererBase.h
            RendererDXVA.h
            RendererHQ.h
            RendererShaders.h
            RendererSoftware.h)

core_add_library(videorenderers-windows)
