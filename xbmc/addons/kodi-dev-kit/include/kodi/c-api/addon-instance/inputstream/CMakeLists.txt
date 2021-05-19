set(HEADERS demux_packet.h
            stream_codec.h
            stream_constants.h
            stream_crypto.h
            timing_constants.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_addon-instance_inputstream)
endif()
