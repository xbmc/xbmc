#
# FindSndio
# ---------
# Finds the Sndio Library
#
# This will define the following variables:
#
# SNDIO_FOUND - system has sndio
# SNDIO_INCLUDE_DIRS - sndio include directory
# SNDIO_DEFINITIONS - sndio definitions
#
# and the following imported targets::
#
#  Sndio::Sndio    - the sndio library
#

find_path(SNDIO_INCLUDE_DIR sndio.h)
find_library(SNDIO_LIBRARY sndio)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sndio
                                  REQUIRED_VARS SNDIO_LIBRARY SNDIO_INCLUDE_DIR)

if(SNDIO_FOUND)
  set(SNDIO_INCLUDE_DIRS ${SNDIO_INCLUDE_DIR})
  set(SNDIO_LIBRARIES ${SNDIO_LIBRARY})
  set(SNDIO_DEFINITIONS -DHAS_SNDIO=1)

  if(NOT TARGET Sndio::Sndio)
    add_library(Sndio::Sndio UNKNOWN IMPORTED)
    set_target_properties(Sndio::Sndio PROPERTIES
                                       IMPORTED_LOCATION "${SNDIO_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${SNDIO_INCLUDE_DIR}")
    set_target_properties(Sndio::Sndio PROPERTIES
                                       INTERFACE_COMPILE_DEFINITIONS -DHAS_SNDIO=1)
  endif()
endif()


mark_as_advanced(SNDIO_INCLUDE_DIR SNDIO_LIBRARY)
