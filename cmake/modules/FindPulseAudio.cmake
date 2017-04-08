#.rst:
# FindPulseAudio
# --------------
# Finds the PulseAudio library
#
# This will define the following variables::
#
#  PULSEAUDIO_FOUND - system has the PulseAudio library
#  PULSEAUDIO_INCLUDE_DIRS - the PulseAudio include directory
#  PULSEAUDIO_LIBRARIES - the libraries needed to use PulseAudio
#  PULSEAUDIO_DEFINITIONS - the definitions needed to use PulseAudio
#
# and the following imported targets::
#
#   PulseAudio::PulseAudio   - The PulseAudio library

if(NOT PulseAudio_FIND_VERSION)
  set(PulseAudio_FIND_VERSION 2.0.0)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PULSEAUDIO libpulse>=${PulseAudio_FIND_VERSION} QUIET)
  pkg_check_modules(PC_PULSEAUDIO_MAINLOOP libpulse-mainloop-glib QUIET)
endif()

find_path(PULSEAUDIO_INCLUDE_DIR NAMES pulse/pulseaudio.h
                                 PATHS ${PC_PULSEAUDIO_INCLUDEDIR} ${PC_PULSEAUDIO_INCLUDE_DIRS})

find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse
                                PATHS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS})

find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop pulse-mainloop-glib libpulse-mainloop-glib
                                         PATHS ${PC_PULSEAUDIO_LIBDIR} ${PC_PULSEAUDIO_LIBRARY_DIRS})

if(PC_PULSEAUDIO_VERSION)
  set(PULSEAUDIO_VERSION_STRING ${PC_PULSEAUDIO_VERSION})
elseif(PULSEAUDIO_INCLUDE_DIR AND EXISTS "${PULSEAUDIO_INCLUDE_DIR}/pulse/version.h")
  file(STRINGS "${PULSEAUDIO_INCLUDE_DIR}/pulse/version.h" pulseaudio_version_str REGEX "^#define[\t ]+pa_get_headers_version\\(\\)[\t ]+\\(\".*\"\\).*")
  string(REGEX REPLACE "^#define[\t ]+pa_get_headers_version\\(\\)[\t ]+\\(\"([^\"]+)\"\\).*" "\\1" PULSEAUDIO_VERSION_STRING "${pulseaudio_version_str}")
  unset(pulseaudio_version_str)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PulseAudio
                                  REQUIRED_VARS PULSEAUDIO_LIBRARY PULSEAUDIO_MAINLOOP_LIBRARY PULSEAUDIO_INCLUDE_DIR
                                  VERSION_VAR PULSEAUDIO_VERSION_STRING)

if(PULSEAUDIO_FOUND)
  set(PULSEAUDIO_INCLUDE_DIRS ${PULSEAUDIO_INCLUDE_DIR})
  set(PULSEAUDIO_LIBRARIES ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY})
  set(PULSEAUDIO_DEFINITIONS -DHAVE_LIBPULSE=1)

  if(NOT TARGET PulseAudio::PulseAudioMainloop)
    add_library(PulseAudio::PulseAudioMainloop UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudioMainloop PROPERTIES
                                                         IMPORTED_LOCATION "${PULSEAUDIO_MAINLOOP_LIBRARY}")
  endif()
  if(NOT TARGET PulseAudio::PulseAudio)
    add_library(PulseAudio::PulseAudio UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudio PROPERTIES
                                                 IMPORTED_LOCATION "${PULSEAUDIO_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${PULSEAUDIO_INCLUDE_DIR}"
                                                 INTERFACE_COMPILE_DEFINITIONS HAVE_LIBPULSE=1
                                                 INTERFACE_LINK_LIBRARIES PulseAudio::PulseAudioMainloop)
  endif()
endif()

mark_as_advanced(PULSEAUDIO_INCLUDE_DIR PULSEAUDIO_LIBRARY PULSEAUDIO_MAINLOOP_LIBRARY)
