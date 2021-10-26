# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  audio_decoder.h
  audio_encoder.h
  game.h
  image_decoder.h
  inputstream.h
  peripheral.h
  pvr.h
  screensaver.h
  vfs.h
  video_codec.h
  visualization.h
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api_addon-instance)
endif()
