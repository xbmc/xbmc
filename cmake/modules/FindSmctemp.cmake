#.rst:
# FindSmctemp
# -------
# Finds the smctemp library
#
# This will define the following imported targets::
#
#   SMCTEMP::SMCTEMP   - The smctemp library

if(NOT TARGET SMCTEMP::SMCTEMP)

  find_path(SMCTEMP_INCLUDE_DIR NAMES smctemp.h
                                PATHS ${PC_SMCTEMP_INCLUDEDIR} NO_CACHE)
  find_library(SMCTEMP_LIBRARY NAMES smctemp
                               PATHS ${PC_SMCTEMP_LIBDIR} NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Smctemp
                                    REQUIRED_VARS SMCTEMP_LIBRARY SMCTEMP_INCLUDE_DIR
                                    VERSION_VAR SMCTEMP_VERSION)

  if(SMCTEMP_FOUND)
    add_library(SMCTEMP::SMCTEMP UNKNOWN IMPORTED)
    set_target_properties(SMCTEMP::SMCTEMP PROPERTIES
                                           IMPORTED_LOCATION "${SMCTEMP_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${SMCTEMP_INCLUDE_DIR}")

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP SMCTEMP::SMCTEMP)
  endif()
endif()
