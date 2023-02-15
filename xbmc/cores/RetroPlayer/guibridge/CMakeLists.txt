set(SOURCES GUIGameMessenger.cpp
            GUIGameRenderManager.cpp
            GUIGameSettings.cpp
            GUIGameSettingsHandle.cpp
            GUIGameVideoHandle.cpp
            GUIRenderHandle.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
)

set(HEADERS GUIGameMessenger.h
            GUIGameRenderManager.h
            GUIGameSettings.h
            GUIGameSettingsHandle.h
            GUIGameVideoHandle.h
            GUIRenderHandle.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            IGameCallback.h
            IGUIRenderSettings.h
            IRenderCallback.h
)

core_add_library(retroplayer_guibridge)
