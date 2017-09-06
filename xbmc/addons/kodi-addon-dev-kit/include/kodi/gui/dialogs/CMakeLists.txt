set(HEADERS ContextMenu.h
            ExtendedProgress.h
            FileBrowser.h
            Keyboard.h
            Numeric.h
            OK.h
            Progress.h
            Select.h
            TextViewer.h
            YesNo.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi_gui_dialogs)
endif()
