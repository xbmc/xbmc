set(HEADERS ChannelGroups.h
            Channels.h
            EDL.h
            EPG.h
            General.h
            MenuHook.h
            Recordings.h
            Stream.h
            Timers.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_addon-instance_pvr)
endif()
