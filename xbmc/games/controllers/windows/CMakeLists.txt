set(SOURCES ControllerInstaller.cpp
            ControllerSelect.cpp
            GUIConfigurationWizard.cpp
            GUIControllerList.cpp
            GUIControllerWindow.cpp
            GUIFeatureList.cpp
            GUIPortList.cpp
            GUIPortWindow.cpp
)

set(HEADERS ControllerInstaller.h
            ControllerSelect.cpp
            GUIConfigurationWizard.h
            GUIControllerDefines.h
            GUIControllerList.h
            GUIControllerWindow.h
            GUIFeatureList.h
            GUIPortDefines.h
            GUIPortList.h
            GUIPortWindow.h
            IConfigurationWindow.h
            IPortList.h
)

core_add_library(games_controller_windows)
