set(SOURCES Addon_GUIControlButton.cpp
            Addon_GUIControlEdit.cpp
            Addon_GUIControlFadeLabel.cpp
            Addon_GUIControlImage.cpp
            Addon_GUIControlLabel.cpp
            Addon_GUIControlProgress.cpp
            Addon_GUIControlRadioButton.cpp
            Addon_GUIControlRendering.cpp
            Addon_GUIControlSettingsSlider.cpp
            Addon_GUIControlSlider.cpp
            Addon_GUIControlSpin.cpp
            Addon_GUIControlTextBox.cpp
            Addon_GUIDialogContextMenu.cpp
            Addon_GUIDialogExtendedProgressBar.cpp
            Addon_GUIDialogFileBrowser.cpp
            Addon_GUIDialogKeyboard.cpp
            Addon_GUIDialogNumeric.cpp
            Addon_GUIDialogOK.cpp
            Addon_GUIDialogProgress.cpp
            Addon_GUIDialogSelect.cpp
            Addon_GUIDialogTextViewer.cpp
            Addon_GUIDialogYesNo.cpp
            Addon_GUIGeneral.cpp
            Addon_GUIListItem.cpp
            Addon_GUIWindow.cpp)

set(HEADERS Addon_GUIControlButton.h
            Addon_GUIControlEdit.h
            Addon_GUIControlFadeLabel.h
            Addon_GUIControlImage.h
            Addon_GUIControlLabel.h
            Addon_GUIControlProgress.h
            Addon_GUIControlRadioButton.h
            Addon_GUIControlRendering.h
            Addon_GUIControlSettingsSlider.h
            Addon_GUIControlSlider.h
            Addon_GUIControlSpin.h
            Addon_GUIControlTextBox.h
            Addon_GUIDialogContextMenu.h
            Addon_GUIDialogExtendedProgressBar.h
            Addon_GUIDialogFileBrowser.h
            Addon_GUIDialogKeyboard.h
            Addon_GUIDialogNumeric.h
            Addon_GUIDialogOK.h
            Addon_GUIDialogProgress.h
            Addon_GUIDialogSelect.h
            Addon_GUIDialogTextViewer.h
            Addon_GUIDialogYesNo.h
            Addon_GUIGeneral.h
            Addon_GUIListItem.h
            Addon_GUIWindow.h
            ../../../../kodi-addon-dev-kit/src/api3/version.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(api3AddonInterfaces_GUI)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(api3AddonInterfaces_GUI ffmpeg)
endif()
