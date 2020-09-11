set(HEADERS context_menu.h
            extended_progress.h
            filebrowser.h
            keyboard.h
            numeric.h
            ok.h
            progress.h
            select.h
            text_viewer.h
            yes_no.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_gui_dialogs)
endif()
