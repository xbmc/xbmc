# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  ChannelGroups.h
  Channels.h
  EDL.h
  EPG.h
  General.h
  MenuHook.h
  Providers.h
  Recordings.h
  Stream.h
  Timers.h
)

if(HEADERS)
  core_add_devkit_header(kodi_addon-instance_pvr)
endif()
