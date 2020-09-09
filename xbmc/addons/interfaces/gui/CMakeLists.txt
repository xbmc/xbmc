set(SOURCES General.cpp
            ListItem.cpp
            Window.cpp)

set(HEADERS General.h
            ListItem.h
            Window.h)

set(UNITY_EXCLUDES AddonCallbacksGUI.cpp
                   AddonGUIRenderingControl.cpp
                   AddonGUIWindow.cpp)

core_add_library(addons_interfaces_gui)
