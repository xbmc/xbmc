#.rst:
# FindPlist
# ---------
# Finds the Plist library
#
# This will define the following target:
#
#   Plist::Plist - The Plist library

if(NOT TARGET Plist::Plist)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_search_module(PC_PLIST libplist-2.0 libplist QUIET)
  endif()

  find_path(PLIST_INCLUDE_DIR plist/plist.h
                              HINTS ${PC_PLIST_INCLUDEDIR}
                              NO_CACHE)

  set(PLIST_VERSION ${PC_PLIST_VERSION})

  find_library(PLIST_LIBRARY NAMES plist-2.0 plist libplist-2.0 libplist
                                   HINTS ${PC_PLIST_LIBDIR}
                                   NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Plist
                                    REQUIRED_VARS PLIST_LIBRARY PLIST_INCLUDE_DIR
                                    VERSION_VAR PLIST_VERSION)

  if(PLIST_FOUND)
    add_library(Plist::Plist UNKNOWN IMPORTED)
    set_target_properties(Plist::Plist PROPERTIES
                                       IMPORTED_LOCATION "${PLIST_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${PLIST_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAS_AIRPLAY=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Plist::Plist)
  endif()
endif()
