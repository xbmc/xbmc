set(SOURCES HdrOutput.cpp
            RenderContext.cpp
            RenderSettings.cpp
            RenderTranslator.cpp
            RenderUtils.cpp
            RenderVideoSettings.cpp
            RPRenderManager.cpp
)

set(HEADERS HdrOutput.h
            IRenderManager.h
            RenderContext.h
            RenderSettings.h
            RenderTranslator.h
            RenderUtils.h
            RenderVideoSettings.h
            RPRenderManager.h
)

core_add_library(rp-rendering)
