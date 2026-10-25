set(SOURCES LutLoader.cpp
            RendererBase.cpp
            RendererDXVA.cpp
            RendererHQ.cpp
            RendererShaders.cpp
            RendererSoftware.cpp)

set(HEADERS LutLoader.h
            RendererBase.h
            RendererDXVA.h
            RendererHQ.h
            RendererShaders.h
            RendererSoftware.h)

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES DolbyVisionEffect.cpp)
  list(APPEND HEADERS DolbyVisionEffect.h)
endif()

core_add_library(videorenderers-windows)
