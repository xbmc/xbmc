#.rst:
# FindGMP
# -----------
# Finds the GNU GMP library
#
# This will will define the following variables::
#
# GMP_FOUND - system has GMP
# GMP_INCLUDE_DIRS - the GMP include directory
# GMP_LIBRARIES - the GMP libraries
#
# and the following imported targets::
#
#   GMP::GMP   - The GMP library

find_path(GMP_INCLUDE_DIR NAMES gmp.h)
find_library(GMP_LIBRARY NAMES gmp libgmp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP
                                  REQUIRED_VARS GMP_LIBRARY GMP_INCLUDE_DIR
                                  VERSION_VAR GMP_VERSION)

if(GMP_FOUND)
  set(GMP_LIBRARIES ${GMP_LIBRARY})
  set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})

  if(NOT TARGET GMP::GMP)
    add_library(GMP::GMP UNKNOWN IMPORTED)
    set_target_properties(GMP::GMP PROPERTIES
                                   IMPORTED_LOCATION "${GMP_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(GMP_INCLUDE_DIR GMP_LIBRARY)
