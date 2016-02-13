set(SOURCES GUIDialogBoxBase.cpp
            GUIDialogBusy.cpp
            GUIDialogButtonMenu.cpp
            GUIDialogCache.cpp
            GUIDialogContextMenu.cpp
            GUIDialogExtendedProgressBar.cpp
            GUIDialogFavourites.cpp
            GUIDialogFileBrowser.cpp
            GUIDialogGamepad.cpp
            GUIDialogKaiToast.cpp
            GUIDialogKeyboardGeneric.cpp
            GUIDialogMediaFilter.cpp
            GUIDialogMediaSource.cpp
            GUIDialogNumeric.cpp
            GUIDialogOK.cpp
            GUIDialogPlayEject.cpp
            GUIDialogPlayerControls.cpp
            GUIDialogProgress.cpp
            GUIDialogSeekBar.cpp
            GUIDialogSelect.cpp
            GUIDialogSimpleMenu.cpp
            GUIDialogSlider.cpp
            GUIDialogSmartPlaylistEditor.cpp
            GUIDialogSmartPlaylistRule.cpp
            GUIDialogSubMenu.cpp
            GUIDialogTextViewer.cpp
            GUIDialogVolumeBar.cpp
            GUIDialogYesNo.cpp)

set(HEADERS GUIDialogBoxBase.h
            GUIDialogBusy.h
            GUIDialogButtonMenu.h
            GUIDialogCache.h
            GUIDialogContextMenu.h
            GUIDialogExtendedProgressBar.h
            GUIDialogFavourites.h
            GUIDialogFileBrowser.h
            GUIDialogGamepad.h
            GUIDialogKaiToast.h
            GUIDialogKeyboardGeneric.h
            GUIDialogMediaFilter.h
            GUIDialogMediaSource.h
            GUIDialogNumeric.h
            GUIDialogOK.h
            GUIDialogPlayEject.h
            GUIDialogPlayerControls.h
            GUIDialogProgress.h
            GUIDialogSeekBar.h
            GUIDialogSelect.h
            GUIDialogSimpleMenu.h
            GUIDialogSlider.h
            GUIDialogSmartPlaylistEditor.h
            GUIDialogSmartPlaylistRule.h
            GUIDialogSubMenu.h
            GUIDialogTextViewer.h
            GUIDialogVolumeBar.h
            GUIDialogYesNo.h)

core_add_library(dialogs)
add_dependencies(dialogs libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dialogs ffmpeg)
endif()
