#.rst:
# FindWebOSHelpers
# --------
# Finds the WebOSHelpers library
#
# This will define the following target:
#
#   WEBOSHELPERS::WEBOSHELPERS   - The webOS helpers library

if(NOT TARGET WEBOSHELPERS::WEBOSHELPERS)

  if(WebOSHelpers_FIND_VERSION)
    if(WebOSHelpers_FIND_VERSION_EXACT)
      set(WebOSHelpers_FIND_SPEC "=${WebOSHelpers_FIND_VERSION_COMPLETE}")
    else()
      set(WebOSHelpers_FIND_SPEC ">=${WebOSHelpers_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_WEBOSHELPERS helpers${WebOSHelpers_FIND_SPEC} QUIET)
  endif()

  find_path(WEBOSHELPERS_INCLUDE_DIR NAMES webos-helpers/libhelpers.h
                                     HINTS ${PC_WEBOSHELPERS_INCLUDEDIR}
                                     NO_CACHE)
  find_library(WEBOSHELPERS_LIBRARY NAMES helpers
                                    HINTS ${PC_WEBOSHELPERS_LIBDIR}
                                    NO_CACHE)

  set(WEBOSHELPERS_VERSION ${PC_WEBOSHELPERS_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(WebOSHelpers
                                    REQUIRED_VARS WEBOSHELPERS_LIBRARY WEBOSHELPERS_INCLUDE_DIR
                                    VERSION_VAR WEBOSHELPERS_VERSION)

  if(WEBOSHELPERS_FOUND)
    add_library(WEBOSHELPERS::WEBOSHELPERS UNKNOWN IMPORTED)
    set_target_properties(WEBOSHELPERS::WEBOSHELPERS PROPERTIES
                                                     IMPORTED_LOCATION "${WEBOSHELPERS_LIBRARY}"
                                                     INTERFACE_INCLUDE_DIRECTORIES "${WEBOSHELPERS_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP WEBOSHELPERS::WEBOSHELPERS)
  endif()
endif()
