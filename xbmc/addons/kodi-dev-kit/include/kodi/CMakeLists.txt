set(HEADERS AddonBase.h
            AudioEngine.h
            Filesystem.h
            General.h
            Network.h
            versions.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES platform/android/System.h)
endif()

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi)
endif()
