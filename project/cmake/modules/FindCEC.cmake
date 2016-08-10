#.rst:
# FindCEC
# -------
# Finds the libCEC library
#
# This will will define the following variables::
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
  pkg_check_modules(PC_CEC libcec>=3.0.0 QUIET)
endif()

find_path(CEC_INCLUDE_DIR libcec/cec.h libCEC/CEC.h
                          PATHS ${PC_CEC_INCLUDEDIR})

set(CEC_VERSION ${PC_CEC_VERSION})

include(FindPackageHandleStandardArgs)
if(NOT WIN32)
  find_library(CEC_LIBRARY NAMES cec
                           PATHS ${PC_CEC_LIBDIR})

  find_package_handle_standard_args(CEC
                                    REQUIRED_VARS CEC_LIBRARY CEC_INCLUDE_DIR
                                    VERSION_VAR CEC_VERSION)
else()
  # Dynamically loaded DLL
  find_package_handle_standard_args(CEC
                                    REQUIRED_VARS CEC_INCLUDE_DIR
                                    VERSION_VAR CEC_VERSION)
endif()

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
