set(SOURCES GUICardinalFeatureButton.cpp
            GUIControllerButton.cpp
            GUIFeatureButton.cpp
            GUIFeatureControls.cpp
            GUIFeatureFactory.cpp
            GUIFeatureTranslator.cpp
            GUIGameController.cpp
            GUIGameControllerList.cpp
            GUIScalarFeatureButton.cpp
            GUISelectKeyButton.cpp
            GUIThrottleButton.cpp
            GUIWheelButton.cpp
)

set(HEADERS GUICardinalFeatureButton.h
            GUIControllerButton.h
            GUIControlTypes.h
            GUIFeatureButton.h
            GUIFeatureControls.h
            GUIFeatureFactory.h
            GUIFeatureTranslator.h
            GUIGameController.h
            GUIGameControllerList.h
            GUIScalarFeatureButton.h
            GUISelectKeyButton.h
            GUIThrottleButton.h
            GUIWheelButton.h
)

core_add_library(games_controller_guicontrols)
