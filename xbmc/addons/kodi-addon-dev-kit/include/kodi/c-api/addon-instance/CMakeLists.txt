set(HEADERS image_decoder.h
            pvr.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi_c-api_addon-instance)
endif()
