#.rst:
# FindPlist
# ---------
# Finds the Plist library
#
# This will define the following variables::
#
# PLIST_FOUND - system has Plist library
# PLIST_INCLUDE_DIRS - the Plist library include directory
# PLIST_LIBRARIES - the Plist libraries
# PLIST_DEFINITIONS - the Plist compile definitions
#
# and the following imported targets::
#
#   Plist::Plist   - The Plist library

if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_PLIST libplist-2.0 libplist QUIET)
endif()

find_path(PLIST_INCLUDE_DIR plist/plist.h
                            PATHS ${PC_PLIST_INCLUDEDIR})

set(PLIST_VERSION ${PC_PLIST_VERSION})

find_library(PLIST_LIBRARY NAMES plist-2.0 plist libplist-2.0 libplist
                                 PATHS ${PC_PLIST_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Plist
                                  REQUIRED_VARS PLIST_LIBRARY PLIST_INCLUDE_DIR
                                  VERSION_VAR PLIST_VERSION)

if(PLIST_FOUND)
  set(PLIST_LIBRARIES ${PLIST_LIBRARY})
  set(PLIST_INCLUDE_DIRS ${PLIST_INCLUDE_DIR})
  set(PLIST_DEFINITIONS -DHAS_AIRPLAY=1)

  if(NOT TARGET Plist::Plist)
    add_library(Plist::Plist UNKNOWN IMPORTED)
    if(PLIST_LIBRARY)
      set_target_properties(Plist::Plist PROPERTIES
                                         IMPORTED_LOCATION "${PLIST_LIBRARY}")
    endif()
    set_target_properties(Plist::Plist PROPERTIES
                                       INTERFACE_INCLUDE_DIRECTORIES "${PLIST_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS HAS_AIRPLAY=1)
  endif()
endif()

mark_as_advanced(PLIST_INCLUDE_DIR PLIST_LIBRARY)
