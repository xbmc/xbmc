# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  button.h
  edit.h
  fade_label.h
  image.h
  label.h
  progress.h
  radio_button.h
  rendering.h
  settings_slider.h
  slider.h
  spin.h
  text_box.h
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api_gui_controls)
endif()
