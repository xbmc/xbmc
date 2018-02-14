set(SOURCES GUIGameRenderManager.cpp
            GUIGameSettings.cpp
            GUIGameVideoHandle.cpp
            GUIRenderHandle.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
)

set(HEADERS GUIGameRenderManager.h
            GUIGameSettings.h
            GUIGameVideoHandle.h
            GUIRenderHandle.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            IGUIRenderSettings.h
            IRenderCallback.h
)

core_add_library(retroplayer_guibridge)
