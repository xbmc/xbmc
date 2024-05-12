#.rst:
# FindPulseAudio
# --------------
# Finds the PulseAudio library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::PulseAudio - The PulseAudio library
#   ${APP_NAME_LC}::PulseAudioSimple - The PulseAudio simple library
#   ${APP_NAME_LC}::PulseAudioMainloop - The PulseAudio mainloop library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    if(PulseAudio_FIND_VERSION)
      if(PulseAudio_FIND_VERSION_EXACT)
        set(PulseAudio_FIND_SPEC "=${PulseAudio_FIND_VERSION_COMPLETE}")
      else()
        set(PulseAudio_FIND_SPEC ">=${PulseAudio_FIND_VERSION_COMPLETE}")
      endif()
    endif()

    pkg_check_modules(PC_PULSEAUDIO libpulse${PulseAudio_FIND_SPEC} QUIET)
    pkg_check_modules(PC_PULSEAUDIO_MAINLOOP libpulse-mainloop-glib${PulseAudio_FIND_SPEC} QUIET)
    pkg_check_modules(PC_PULSEAUDIO_SIMPLE libpulse-simple${PulseAudio_FIND_SPEC} QUIET)
  endif()

  find_path(PULSEAUDIO_INCLUDE_DIR NAMES pulse/pulseaudio.h pulse/simple.h
                                   HINTS ${PC_PULSEAUDIO_INCLUDEDIR} ${PC_PULSEAUDIO_INCLUDE_DIRS})

  find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse
                                  HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS})

  find_library(PULSEAUDIO_SIMPLE_LIBRARY NAMES pulse-simple libpulse-simple
                                         HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS})

  find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop pulse-mainloop-glib libpulse-mainloop-glib
                                           HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS})

  if(PC_PULSEAUDIO_VERSION)
    set(PULSEAUDIO_VERSION_STRING ${PC_PULSEAUDIO_VERSION})
  elseif(PULSEAUDIO_INCLUDE_DIR AND EXISTS "${PULSEAUDIO_INCLUDE_DIR}/pulse/version.h")
    file(STRINGS "${PULSEAUDIO_INCLUDE_DIR}/pulse/version.h" pulseaudio_version_str REGEX "^#define[\t ]+pa_get_headers_version\\(\\)[\t ]+\\(\".*\"\\).*")
    string(REGEX REPLACE "^#define[\t ]+pa_get_headers_version\\(\\)[\t ]+\\(\"([^\"]+)\"\\).*" "\\1" PULSEAUDIO_VERSION_STRING "${pulseaudio_version_str}")
    unset(pulseaudio_version_str)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PulseAudio
                                    REQUIRED_VARS PULSEAUDIO_LIBRARY PULSEAUDIO_MAINLOOP_LIBRARY PULSEAUDIO_SIMPLE_LIBRARY PULSEAUDIO_INCLUDE_DIR
                                    VERSION_VAR PULSEAUDIO_VERSION_STRING)

  if(PULSEAUDIO_FOUND)
    list(APPEND AUDIO_BACKENDS_LIST "pulseaudio")
    set(AUDIO_BACKENDS_LIST ${AUDIO_BACKENDS_LIST} PARENT_SCOPE)

    add_library(${APP_NAME_LC}::PulseAudioSimple UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::PulseAudioSimple PROPERTIES
                                                           IMPORTED_LOCATION "${PULSEAUDIO_SIMPLE_LIBRARY}")

    add_library(${APP_NAME_LC}::PulseAudioMainloop UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::PulseAudioMainloop PROPERTIES
                                                             IMPORTED_LOCATION "${PULSEAUDIO_MAINLOOP_LIBRARY}")

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${PULSEAUDIO_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${PULSEAUDIO_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_PULSEAUDIO
                                                                     INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::PulseAudioMainloop;${APP_NAME_LC}::PulseAudioSimple")
  endif()
endif()
