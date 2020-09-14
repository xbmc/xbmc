set(HEADERS addon_base.h
            audio_engine.h
            filesystem.h
            general.h
            network.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES platform/android/system.h)
endif()

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_c-api)
endif()
