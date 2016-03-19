# - Try to find ALSA
# Once done this will define
#
# ALSA_FOUND - system has libALSA
# ALSA_INCLUDE_DIRS - the libALSA include directory
# ALSA_LIBRARIES - The libALSA libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (ALSA alsa)
else()
  find_path(ALSA_INCLUDE_DIRS asoundlib.h PATH_SUFFIXES alsa)
  find_library(ALSA_LIBRARIES asound)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Alsa DEFAULT_MSG ALSA_INCLUDE_DIRS ALSA_LIBRARIES)

set(ALSA_INCLUDE_DIRS "") # Dont want these added as 'timer.h' is a dangerous file
mark_as_advanced(ALSA_INCLUDE_DIRS ALSA_LIBRARIES)
list(APPEND ALSA_DEFINITIONS -DHAVE_ALSA=1 -DUSE_ALSA=1)
