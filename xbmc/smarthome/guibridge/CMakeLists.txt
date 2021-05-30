set(SOURCES GUIRenderHandle.cpp
            GUIRenderSettings.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
            SmartHomeGuiBridge.cpp
            SmartHomeGuiManager.cpp
)

set(HEADERS GUIRenderHandle.h
            GUIRenderSettings.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            SmartHomeGuiBridge.h
            SmartHomeGuiManager.h
)

core_add_library(smarthome_guibridge)
