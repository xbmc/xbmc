set(HEADERS Button.h
            Edit.h
            FadeLabel.h
            Image.h
            Label.h
            Progress.h
            RadioButton.h
            Rendering.h
            SettingsSlider.h
            Slider.h
            Spin.h
            TextBox.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_gui_controls)
endif()
