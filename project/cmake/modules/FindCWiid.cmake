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

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CWIID cwiid QUIET)
endif()

find_path(CWIID_INCLUDE_DIR NAMES cwiid.h
                            PATHS ${PC_CWIID_INCLUDEDIR})
find_library(CWIID_LIBRARY NAMES cwiid
                           PATHS ${PC_CWIID_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CWIID
                                  REQUIRED_VARS CWIID_LIBRARY CWIID_INCLUDE_DIR)

if(CWIID_FOUND)
  set(CWIID_INCLUDE_DIRS ${CWIID_INCLUDE_DIR})
  set(CWIID_LIBRARIES ${CWIID_LIBRARY})
endif()

mark_as_advanced(CWIID_INCLUDE_DIR CWIID_LIBRARY)
