set(HEADERS definitions.h
            general.h
            list_item.h
            window.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_gui)
endif()
