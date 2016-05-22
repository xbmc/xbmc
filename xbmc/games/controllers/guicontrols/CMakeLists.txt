set(SOURCES GUIAnalogStickButton.cpp
            GUIControllerButton.cpp
            GUIFeatureButton.cpp
            GUIFeatureControls.cpp
            GUIGameController.cpp
            GUIScalarFeatureButton.cpp)

set(HEADERS GUIAnalogStickButton.h
            GUIControllerButton.h
            GUIFeatureButton.h
            GUIFeatureControls.h
            GUIGameController.h
            GUIScalarFeatureButton.h)

core_add_library(games_controller_guicontrols)
