# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  pvr_channel_groups.h
  pvr_channels.h
  pvr_defines.h
  pvr_edl.h
  pvr_epg.h
  pvr_general.h
  pvr_menu_hook.h
  pvr_providers.h
  pvr_recordings.h
  pvr_stream.h
  pvr_timers.h
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api_addon-instance_pvr)
endif()
