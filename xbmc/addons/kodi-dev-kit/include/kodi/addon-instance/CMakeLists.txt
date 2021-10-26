# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  AudioDecoder.h
  AudioEncoder.h
  Game.h
  ImageDecoder.h
  Inputstream.h
  PVR.h
  Peripheral.h
  Screensaver.h
  VFS.h
  VideoCodec.h
  Visualization.h
)

if(HEADERS)
  core_add_devkit_header(kodi_addon-instance)
endif()
