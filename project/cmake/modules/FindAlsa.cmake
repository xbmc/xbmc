#.rst:
# FindAlsa
# --------
# Finds the Alsa library
#
# This will will define the following variables::
#
# ALSA_FOUND - system has Alsa
# ALSA_INCLUDE_DIRS - the Alsa include directory
# ALSA_LIBRARIES - the Alsa libraries
# ALSA_DEFINITIONS - the Alsa compile definitions
#
# and the following imported targets::
#
#   ALSA::ALSA   - The Alsa library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ALSA alsa QUIET)
endif()

find_path(ALSA_INCLUDE_DIR NAMES alsa/asoundlib.h
                           PATHS ${PC_ALSA_INCLUDEDIR})
find_library(ALSA_LIBRARY NAMES asound
                          PATHS ${PC_ALSA_LIBDIR})

set(ALSA_VERSION ${PC_ALSA_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALSA
                                  REQUIRED_VARS ALSA_LIBRARY ALSA_INCLUDE_DIR
                                  VERSION_VAR ALSA_VERSION)

if(ALSA_FOUND)
  set(ALSA_INCLUDE_DIRS "") # Don't want these added as 'timer.h' is a dangerous file
  set(ALSA_LIBRARIES ${ALSA_LIBRARY})
  set(ALSA_DEFINITIONS -DHAVE_ALSA=1 -DUSE_ALSA=1)

  if(NOT TARGET ALSA::ALSA)
    add_library(ALSA::ALSA UNKNOWN IMPORTED)
    set_target_properties(ALSA::ALSA PROPERTIES
                                     IMPORTED_LOCATION "${ALSA_LIBRARY}"
                                     INTERFACE_COMPILE_DEFINITIONS "${ALSA_DEFINITIONS}")
  endif()
endif()

mark_as_advanced(ALSA_INCLUDE_DIR ALSA_LIBRARY)
