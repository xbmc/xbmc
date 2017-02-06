#.rst:
# FindJEMALLOC
# -------
# Finds the JEMALLOC library
#
# This will will define the following variables::
#
# JEMALLOC_FOUND - system has JEMALLOC
# JEMALLOC_INCLUDE_DIRS - the JEMALLOC include directory
# JEMALLOC_LIBRARIES - the JEMALLOC libraries
#
# and the following imported targets::
#
#   JEMALLOC::JEMALLOC   - The JEMALLOC library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_JEMALLOC libjemalloc QUIET)
endif()

find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc/jemalloc.h
                          PATHS ${PC_JEMALLOC_INCLUDEDIR})
find_library(JEMALLOC_LIBRARY NAMES jemalloc_pic libjemalloc_pic
                         PATHS ${PC_JEMALLOC_LIBDIR})

set(JEMALLOC_VERSION ${PC_JEMALLOC_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JEMALLOC
                                  REQUIRED_VARS JEMALLOC_LIBRARY JEMALLOC_INCLUDE_DIR
                                  VERSION_VAR JEMALLOC_VERSION)

if(JEMALLOC_FOUND)
  set(JEMALLOC_LIBRARIES ${JEMALLOC_LIBRARY})
  set(JEMALLOC_INCLUDE_DIRS ${JEMALLOC_INCLUDE_DIR})

  if(NOT TARGET JEMALLOC::JEMALLOC)
    add_library(JEMALLOC::JEMALLOC UNKNOWN IMPORTED)
    set_target_properties(JEMALLOC::JEMALLOC PROPERTIES
                                   IMPORTED_LOCATION "${JEMALLOC_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${JEMALLOC_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(JEMALLOC_INCLUDE_DIR JEMALLOC_LIBRARY)
