set(SOURCES ControllerInstaller.cpp
            ControllerSelect.cpp
            GUIDialogAxisDetection.cpp
            GUIDialogButtonCapture.cpp
            GUIDialogIgnoreInput.cpp
)

set(HEADERS ControllerInstaller.h
            ControllerSelect.h
            GUIDialogAxisDetection.h
            GUIDialogButtonCapture.h
            GUIDialogIgnoreInput.h
)

core_add_library(games_controller_dialogs)
