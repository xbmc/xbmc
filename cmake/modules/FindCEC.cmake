#.rst:
# FindCEC
# -------
# Finds the libCEC library
#
# This will define the following target:
#
#   CEC::CEC - The libCEC library

if(NOT TARGET CEC::CEC)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CEC libcec QUIET)
  endif()

  find_path(CEC_INCLUDE_DIR NAMES libcec/cec.h libCEC/CEC.h
                            PATHS ${PC_CEC_INCLUDEDIR}
                            NO_CACHE)

  if(PC_CEC_VERSION)
    set(CEC_VERSION ${PC_CEC_VERSION})
  elseif(CEC_INCLUDE_DIR AND EXISTS "${CEC_INCLUDE_DIR}/libcec/version.h")
    file(STRINGS "${CEC_INCLUDE_DIR}/libcec/version.h" cec_version_str REGEX "^[\t ]+LIBCEC_VERSION_TO_UINT\\(.*\\)")
    string(REGEX REPLACE "^[\t ]+LIBCEC_VERSION_TO_UINT\\(([0-9]+), ([0-9]+), ([0-9]+)\\)" "\\1.\\2.\\3" CEC_VERSION "${cec_version_str}")
    unset(cec_version_str)
  endif()

  if(NOT CEC_FIND_VERSION)
    set(CEC_FIND_VERSION 4.0.0)
  endif()

  find_library(CEC_LIBRARY NAMES cec
                           PATHS ${PC_CEC_LIBDIR}
                           NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CEC
                                    REQUIRED_VARS CEC_LIBRARY CEC_INCLUDE_DIR
                                    VERSION_VAR CEC_VERSION)

  if(CEC_FOUND)
    add_library(CEC::CEC UNKNOWN IMPORTED)
    set_target_properties(CEC::CEC PROPERTIES
                                   IMPORTED_LOCATION "${CEC_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${CEC_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCEC=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CEC::CEC)
  endif()
endif()
