#.rst:
# FindCdio
# --------
# Finds the cdio library
#
# This will will define the following variables::
#
# CDIO_FOUND - system has cdio
# CDIO_INCLUDE_DIRS - the cdio include directory
# CDIO_LIBRARIES - the cdio libraries
#
# and the following imported targets::
#
#   CDIO::CDIO - The cdio library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CDIO libcdio libiso9660 QUIET)
endif()

find_path(CDIO_INCLUDE_DIR NAMES cdio/cdio.h
                           PATHS ${PC_CDIO_libcdio_INCLUDEDIR}
                                 ${PC_CDIO_libiso9660_INCLUDEDIR})
find_library(CDIO_LIBRARY NAMES cdio
                          PATHS ${CDIO_libcdio_LIBDIR} ${CDIO_libiso9660_LIBDIR})

set(CDIO_VERSION ${PC_CDIO_libcdio_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CDIO
                                  REQUIRED_VARS CDIO_LIBRARY CDIO_INCLUDE_DIR
                                  VERSION_VAR CDIO_VERSION)

if(CDIO_FOUND)
  set(CDIO_LIBRARIES ${CDIO_LIBRARY})
  set(CDIO_INCLUDE_DIRS ${CDIO_INCLUDE_DIR})

  if(NOT TARGET CDIO::CDIO)
    add_library(CDIO::CDIO UNKNOWN IMPORTED)
    set_target_properties(CDIO::CDIO PROPERTIES
                                     IMPORTED_LOCATION "${CDIO_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${CDIO_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(CDIO_INCLUDE_DIR CDIO_LIBRARY)
