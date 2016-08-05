#.rst:
# FindLibDRM
# ----------
# Finds the LibDRM library
#
# This will will define the following variables::
#
# LIBDRM_FOUND - system has LibDRM
# LIBDRM_INCLUDE_DIRS - the LibDRM include directory
# LIBDRM_LIBRARIES - the LibDRM libraries
#
# and the following imported targets::
#
#   LibDRM::LibDRM   - The LibDRM library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_LIBDRM libdrm QUIET)
endif()

find_path(LIBDRM_INCLUDE_DIR NAMES drm.h
                             PATH_SUFFIXES libdrm drm
                             PATHS ${PC_LIBDRM_INCLUDEDIR})
find_library(LIBDRM_LIBRARY NAMES drm
                            PATHS ${PC_LIBDRM_LIBDIR})

set(LIBDRM_VERSION ${PC_LIBDRM_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDRM
                                  REQUIRED_VARS LIBDRM_LIBRARY LIBDRM_INCLUDE_DIR
                                  VERSION_VAR LIBDRM_VERSION)

if(LIBDRM_FOUND)
  set(LIBDRM_LIBRARIES ${LIBDRM_LIBRARY})
  set(LIBDRM_INCLUDE_DIRS ${LIBDRM_INCLUDE_DIR})

  if(NOT TARGET LIBDRM::LIBDRM)
    add_library(LIBDRM::LIBDRM UNKNOWN IMPORTED)
    set_target_properties(LIBDRM::LIBDRM PROPERTIES
                                   IMPORTED_LOCATION "${LIBDRM_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(LIBDRM_INCLUDE_DIR LIBDRM_LIBRARY)
