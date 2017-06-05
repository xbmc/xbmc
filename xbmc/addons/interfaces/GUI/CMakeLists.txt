set(SOURCES General.cpp
            ListItem.cpp
            Window.cpp
            AddonCallbacksGUI.cpp
            AddonGUIRenderingControl.cpp
            AddonGUIWindow.cpp)

set(HEADERS General.h
            ListItem.h
            Window.h
            AddonCallbacksGUI.h
            AddonGUIRenderingControl.h
            AddonGUIWindow.h)

core_add_library(addonInterface_GUI)
