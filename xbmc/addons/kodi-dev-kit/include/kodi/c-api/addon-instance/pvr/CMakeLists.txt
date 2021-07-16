set(HEADERS pvr_channel_groups.h
            pvr_providers.h
            pvr_channels.h
            pvr_defines.h
            pvr_edl.h
            pvr_epg.h
            pvr_general.h
            pvr_menu_hook.h
            pvr_recordings.h
            pvr_stream.h
            pvr_timers.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_addon-instance_pvr)
endif()
