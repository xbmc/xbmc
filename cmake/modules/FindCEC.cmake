#.rst:
# FindCEC
# -------
# Finds the libCEC library
#
# This will define the following variables::
#
# CEC_FOUND - system has libCEC
# CEC_INCLUDE_DIRS - the libCEC include directory
# CEC_LIBRARIES - the libCEC libraries
# CEC_DEFINITIONS - the libCEC compile definitions
#
# and the following imported targets::
#
#   CEC::CEC   - The libCEC library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CEC libcec QUIET)
endif()

find_path(CEC_INCLUDE_DIR NAMES libcec/cec.h libCEC/CEC.h
                          PATHS ${PC_CEC_INCLUDEDIR})

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
                         PATHS ${PC_CEC_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CEC
                                  REQUIRED_VARS CEC_LIBRARY CEC_INCLUDE_DIR
                                  VERSION_VAR CEC_VERSION)

if(CEC_FOUND)
  set(CEC_LIBRARIES ${CEC_LIBRARY})
  set(CEC_INCLUDE_DIRS ${CEC_INCLUDE_DIR})
  set(CEC_DEFINITIONS -DHAVE_LIBCEC=1)

  if(NOT TARGET CEC::CEC)
    add_library(CEC::CEC UNKNOWN IMPORTED)
    if(CEC_LIBRARY)
      set_target_properties(CEC::CEC PROPERTIES
                                     IMPORTED_LOCATION "${CEC_LIBRARY}")
    endif()
    set_target_properties(CEC::CEC PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${CEC_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCEC=1)
  endif()
endif()

mark_as_advanced(CEC_INCLUDE_DIR CEC_LIBRARY)
