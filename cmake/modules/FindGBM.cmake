# FindGBM
# ----------
# Finds the GBM library
#
# This will will define the following variables::
#
# GBM_FOUND - system has GBM
# GBM_INCLUDE_DIRS - the GBM include directory
# GBM_LIBRARIES - the GBM libraries
# GBM_DEFINITIONS  - the GBM definitions
#
# and the following imported targets::
#
#   GBM::GBM   - The GBM library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GBM gbm QUIET)
endif()

find_path(GBM_INCLUDE_DIR NAMES gbm.h
                          PATHS ${PC_GBM_INCLUDEDIR})
find_library(GBM_LIBRARY NAMES gbm
                         PATHS ${PC_GBM_LIBDIR})

set(GBM_VERSION ${PC_GBM_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GBM
                                  REQUIRED_VARS GBM_LIBRARY GBM_INCLUDE_DIR
                                  VERSION_VAR GBM_VERSION)

if(GBM_FOUND)
  set(GBM_LIBRARIES ${GBM_LIBRARY})
  set(GBM_INCLUDE_DIRS ${GBM_INCLUDE_DIR})
  set(GBM_DEFINITIONS -DHAVE_GBM=1)
    if(NOT TARGET GBM::GBM)
    add_library(GBM::GBM UNKNOWN IMPORTED)
    set_target_properties(GBM::GBM PROPERTIES
                                   IMPORTED_LOCATION "${GBM_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GBM_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(GBM_INCLUDE_DIR GBM_LIBRARY)
