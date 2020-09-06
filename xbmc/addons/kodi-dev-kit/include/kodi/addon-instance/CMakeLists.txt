set(HEADERS AudioDecoder.h
            AudioEncoder.h
            Game.h
            ImageDecoder.h
            Inputstream.h
            Peripheral.h
            PVR.h
            Screensaver.h
            VFS.h
            VideoCodec.h
            Visualization.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_addon-instance)
endif()
