set(SOURCES RendererBase.cpp
            RendererDXVA.cpp
            RendererHQ.cpp
            RendererShaders.cpp
            RendererSoftware.cpp
            RendererPL.cpp
            rendererlibplacebo.cpp
            libav_wrapper.c
            PlHelper.cpp)

set(HEADERS RendererBase.h
            RendererDXVA.h
            RendererHQ.h
            RendererShaders.h
            RendererSoftware.h            
            RendererPL.h
            rendererlibplacebo.h
            PlHelper.h)
core_add_library(videorenderers-windows)
