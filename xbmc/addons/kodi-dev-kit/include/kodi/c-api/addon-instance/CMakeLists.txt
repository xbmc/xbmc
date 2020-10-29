set(HEADERS audio_decoder.h
            audio_encoder.h
            game.h
            image_decoder.h
            inputstream.h
            peripheral.h
            pvr.h
            screensaver.h
            vfs.h
            video_codec.h
            visualization.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api_addon-instance)
endif()
