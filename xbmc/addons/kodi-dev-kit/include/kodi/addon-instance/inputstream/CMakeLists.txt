# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  DemuxPacket.h
  StreamCodec.h
  StreamConstants.h
  StreamCrypto.h
  TimingConstants.h
)

if(HEADERS)
  core_add_devkit_header(kodi_addon-instance_inputstream)
endif()
