set(HEADERS General.h
            ListItem.h
            Window.h
            definitions.h
            renderHelper.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-addon-dev-kit_include_kodi_gui)
endif()
