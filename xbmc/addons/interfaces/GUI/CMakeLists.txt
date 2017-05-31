set(SOURCES General.cpp
            DialogContextMenu.cpp
            DialogExtendedProgressBar.cpp
            DialogFileBrowser.cpp
            DialogKeyboard.cpp
            DialogNumeric.cpp
            DialogOK.cpp
            DialogProgress.cpp
            DialogSelect.cpp
            DialogTextViewer.cpp
            DialogYesNo.cpp
            AddonCallbacksGUI.cpp
            AddonGUIRenderingControl.cpp
            AddonGUIWindow.cpp)

set(HEADERS General.h
            DialogContextMenu.h
            DialogExtendedProgressBar.h
            DialogFileBrowser.h
            DialogKeyboard.h
            DialogNumeric.h
            DialogOK.h
            DialogProgress.h
            DialogSelect.h
            DialogTextViewer.h
            DialogYesNo.h
            AddonCallbacksGUI.h
            AddonGUIRenderingControl.h
            AddonGUIWindow.h)

core_add_library(addonInterface_GUI)
