set(SOURCES GUIConfigurationWizard.cpp
            GUIControllerList.cpp
            GUIControllerWindow.cpp
            GUIFeatureList.cpp)

set(HEADERS GUIConfigurationWizard.h
            GUIControllerDefines.h
            GUIControllerList.h
            GUIControllerWindow.h
            GUIFeatureList.h
            IConfigurationWindow.h)

core_add_library(games_controller_windows)
