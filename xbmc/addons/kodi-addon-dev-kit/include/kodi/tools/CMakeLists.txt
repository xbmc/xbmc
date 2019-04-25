set(HEADERS DllHelper.h
            Time.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi_tools)
endif()
