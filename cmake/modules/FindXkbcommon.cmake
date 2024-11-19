# FindXkbcommon
# -----------
# Finds the libxkbcommon library
#
# This will define the following target:
#
#   XKBCOMMON::XKBCOMMON   - The libxkbcommon library

if(NOT TARGET XKBCOMMON::XKBCOMMON)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_XKBCOMMON xkbcommon QUIET)
  endif()

  find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h
                                  HINTS ${PC_XKBCOMMON_INCLUDEDIR}
                                  NO_CACHE)
  find_library(XKBCOMMON_LIBRARY NAMES xkbcommon
                                 HINTS ${PC_XKBCOMMON_LIBDIR}
                                 NO_CACHE)

  set(XKBCOMMON_VERSION ${PC_XKBCOMMON_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Xkbcommon
                                    REQUIRED_VARS XKBCOMMON_LIBRARY XKBCOMMON_INCLUDE_DIR
                                    VERSION_VAR XKBCOMMON_VERSION)

  if(XKBCOMMON_FOUND)
    add_library(XKBCOMMON::XKBCOMMON UNKNOWN IMPORTED)
    set_target_properties(XKBCOMMON::XKBCOMMON PROPERTIES
                                               IMPORTED_LOCATION "${XKBCOMMON_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${XKBCOMMON_INCLUDE_DIR}")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP XKBCOMMON::XKBCOMMON)
  endif()
endif()
