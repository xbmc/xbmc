#.rst:
# FindCWiid
# ---------
# Finds the CWiid library
#
# This will will define the following variables::
#
# CWIID_FOUND - system has CWiid
# CWIID_INCLUDE_DIRS - the CWiid include directory
# CWIID_LIBRARIES - the CWiid libraries
#
# and the following imported targets::
#
#   CWiid::CWiid   - The CWiid library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CWIID cwiid QUIET)
endif()

find_path(CWIID_INCLUDE_DIR NAMES cwiid.h
                            PATHS ${PC_CWIID_INCLUDEDIR})
find_library(CWIID_LIBRARY NAMES cwiid
                           PATHS ${PC_CWIID_LIBDIR})

set(CWIID_VERSION ${PC_CWIID_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CWIID
                                  REQUIRED_VARS CWIID_LIBRARY CWIID_INCLUDE_DIR
                                  VERSION_VAR CWIID_VERSION)

if(CWIID_FOUND)
  set(CWIID_INCLUDE_DIRS ${CWIID_INCLUDE_DIR})
  set(CWIID_LIBRARIES ${CWIID_LIBRARY})

  if(NOT TARGET CWiid::CWiid)
    add_library(CWiid::CWiid UNKNOWN IMPORTED)
    set_target_properties(CWiid::CWiid PROPERTIES
                                       IMPORTED_LOCATION "${CWIID_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${CWIID_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(CWIID_INCLUDE_DIR CWIID_LIBRARY)
