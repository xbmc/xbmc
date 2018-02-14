set(SOURCES RenderContext.cpp
            RenderGeometry.cpp
            RenderSettings.cpp
            RenderTranslator.cpp
            RenderVideoSettings.cpp
            RPRenderManager.cpp
)

set(HEADERS IRenderManager.h
            RenderContext.h
            RenderGeometry.h
            RenderSettings.h
            RenderTranslator.h
            RenderVideoSettings.h
            RPRenderManager.h
)

core_add_library(rp-rendering)
