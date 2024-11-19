#.rst:
# FindPulseAudio
# --------------
# Finds the PulseAudio library
#
# This will define the following target:
#
#   PulseAudio::PulseAudio - The PulseAudio library
#   PulseAudio::PulseAudioSimple - The PulseAudio simple library
#   PulseAudio::PulseAudioMainloop - The PulseAudio mainloop library

if(NOT TARGET PulseAudio::PulseAudio)

  if(PulseAudio_FIND_VERSION)
    if(PulseAudio_FIND_VERSION_EXACT)
      set(PulseAudio_FIND_SPEC "=${PulseAudio_FIND_VERSION_COMPLETE}")
    else()
      set(PulseAudio_FIND_SPEC ">=${PulseAudio_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PULSEAUDIO libpulse${PulseAudio_FIND_SPEC} QUIET)
    pkg_check_modules(PC_PULSEAUDIO_MAINLOOP libpulse-mainloop-glib${PulseAudio_FIND_SPEC} QUIET)
    pkg_check_modules(PC_PULSEAUDIO_SIMPLE libpulse-simple${PulseAudio_FIND_SPEC} QUIET)
  endif()

  find_path(PULSEAUDIO_INCLUDE_DIR NAMES pulse/pulseaudio.h pulse/simple.h
                                   HINTS ${PC_PULSEAUDIO_INCLUDEDIR} ${PC_PULSEAUDIO_INCLUDE_DIRS}
                                   NO_CACHE)

  find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse
                                  HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS}
                                  NO_CACHE)

  find_library(PULSEAUDIO_SIMPLE_LIBRARY NAMES pulse-simple libpulse-simple
                                         HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS}
                                         NO_CACHE)

  find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop pulse-mainloop-glib libpulse-mainloop-glib
                                           HINTS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS}
                                           NO_CACHE)

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

    add_library(PulseAudio::PulseAudioSimple UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudioSimple PROPERTIES
                                                       IMPORTED_LOCATION "${PULSEAUDIO_SIMPLE_LIBRARY}")

    add_library(PulseAudio::PulseAudioMainloop UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudioMainloop PROPERTIES
                                                         IMPORTED_LOCATION "${PULSEAUDIO_MAINLOOP_LIBRARY}")

    add_library(PulseAudio::PulseAudio UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudio PROPERTIES
                                                 IMPORTED_LOCATION "${PULSEAUDIO_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${PULSEAUDIO_INCLUDE_DIR}"
                                                 INTERFACE_COMPILE_DEFINITIONS HAS_PULSEAUDIO=1
                                                 INTERFACE_LINK_LIBRARIES "PulseAudio::PulseAudioMainloop;PulseAudio::PulseAudioSimple")

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP PulseAudio::PulseAudio)
  endif()
endif()
