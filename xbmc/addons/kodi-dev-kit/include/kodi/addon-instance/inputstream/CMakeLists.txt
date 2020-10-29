set(HEADERS DemuxPacket.h
            StreamCodec.h
            StreamConstants.h
            TimingConstants.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_addon-instance_inputstream)
endif()
