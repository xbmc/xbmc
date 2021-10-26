# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  context_menu.h
  extended_progress.h
  filebrowser.h
  keyboard.h
  numeric.h
  ok.h
  progress.h
  select.h
  text_viewer.h
  yes_no.h
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api_gui_dialogs)
endif()
