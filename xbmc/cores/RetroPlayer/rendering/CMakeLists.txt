set(SOURCES GUIGameRenderManager.cpp
            GUIGameSettings.cpp
            GUIGameVideoHandle.cpp
            GUIRenderHandle.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
            RenderContext.cpp
            RenderGeometry.cpp
            RenderSettings.cpp
            RenderVideoSettings.cpp
            RPRenderManager.cpp
)

set(HEADERS GUIGameRenderManager.h
            GUIGameSettings.h
            GUIGameVideoHandle.h
            GUIRenderHandle.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            IGUIRenderSettings.h
            IRenderCallback.h
            IRenderManager.h
            RenderContext.h
            RenderGeometry.h
            RenderSettings.h
            RenderVideoSettings.h
            RPRenderManager.h
)

core_add_library(rp-rendering)
