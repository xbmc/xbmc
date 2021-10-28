# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  addon_base.h
  audio_engine.h
  filesystem.h
  general.h
  network.h
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api)
endif()
