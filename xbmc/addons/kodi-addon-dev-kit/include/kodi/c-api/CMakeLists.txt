set(HEADERS addon_base.h
            audio_engine.h
            filesystem.h
            general.h
            network.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi_c-api)
endif()
