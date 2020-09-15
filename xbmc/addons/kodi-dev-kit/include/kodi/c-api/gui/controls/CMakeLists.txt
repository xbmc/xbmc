set(HEADERS button.h
            edit.h
            fade_label.h
            image.h
            label.h
            progress.h
            radio_button.h
            rendering.h
            settings_slider.h
            slider.h
            spin.h
            text_box.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_gui_controls)
endif()
