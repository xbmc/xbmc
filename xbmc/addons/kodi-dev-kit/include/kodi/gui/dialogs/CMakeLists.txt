# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  ContextMenu.h
  ExtendedProgress.h
  FileBrowser.h
  Keyboard.h
  Numeric.h
  OK.h
  Progress.h
  Select.h
  TextViewer.h
  YesNo.h
)

if(HEADERS)
  core_add_devkit_header(kodi_gui_dialogs)
endif()
