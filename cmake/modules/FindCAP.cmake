#.rst:
# FindCAP
# -----------
# Finds the POSIX 1003.1e capabilities library
#
# This will define the following target:
#
# CAP::CAP - The LibCap library

if(NOT TARGET CAP::CAP)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CAP libcap QUIET)
  endif()

  find_path(CAP_INCLUDE_DIR NAMES sys/capability.h
                            HINTS ${PC_CAP_INCLUDEDIR}
                            NO_CACHE)
  find_library(CAP_LIBRARY NAMES cap libcap
                           HINTS ${PC_CAP_LIBDIR}
                           NO_CACHE)

  set(CAP_VERSION ${PC_CAP_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CAP
                                    REQUIRED_VARS CAP_LIBRARY CAP_INCLUDE_DIR
                                    VERSION_VAR CAP_VERSION)

  if(CAP_FOUND)
    add_library(CAP::CAP UNKNOWN IMPORTED)
    set_target_properties(CAP::CAP PROPERTIES
                                   IMPORTED_LOCATION "${CAP_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${CAP_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCAP=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CAP::CAP)
  endif()
endif()
