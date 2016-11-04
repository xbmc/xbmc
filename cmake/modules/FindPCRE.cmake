#.rst:
# FindPCRE
# --------
# Finds the PCRECPP library
#
# This will will define the following variables::
#
# PCRE_FOUND - system has libpcrecpp
# PCRE_INCLUDE_DIRS - the libpcrecpp include directory
# PCRE_LIBRARIES - the libpcrecpp libraries
# PCRE_DEFINITIONS - the libpcrecpp definitions
#
# and the following imported targets::
#
#   PCRE::PCRECPP - The PCRECPP library
#   PCRE::PCRE    - The PCRE library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PCRE libpcrecpp QUIET)
endif()

find_path(PCRE_INCLUDE_DIR pcrecpp.h
                           PATHS ${PC_PCRE_INCLUDEDIR})
find_library(PCRECPP_LIBRARY_RELEASE NAMES pcrecpp
                                     PATHS ${PC_PCRE_LIBDIR})
find_library(PCRE_LIBRARY_RELEASE NAMES pcre
                                  PATHS ${PC_PCRE_LIBDIR})
find_library(PCRECPP_LIBRARY_DEBUG NAMES pcrecppd
                                   PATHS ${PC_PCRE_LIBDIR})
find_library(PCRE_LIBRARY_DEBUG NAMES pcred
                                   PATHS ${PC_PCRE_LIBDIR})
set(PCRE_VERSION ${PC_PCRE_VERSION})

include(SelectLibraryConfigurations)
select_library_configurations(PCRECPP)
select_library_configurations(PCRE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE
                                  REQUIRED_VARS PCRECPP_LIBRARY PCRE_LIBRARY PCRE_INCLUDE_DIR
                                  VERSION_VAR PCRE_VERSION)

if(PCRE_FOUND)
  set(PCRE_LIBRARIES ${PCRECPP_LIBRARY} ${PCRE_LIBRARY})
  set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
  if(WIN32)
    set(PCRE_DEFINITIONS -DPCRE_STATIC=1)
  endif()

  if(NOT TARGET PCRE::PCRE)
    add_library(PCRE::PCRE UNKNOWN IMPORTED)
    if(PCRE_LIBRARY_RELEASE)
      set_target_properties(PCRE::PCRE PROPERTIES
                                       IMPORTED_CONFIGURATIONS RELEASE
                                       IMPORTED_LOCATION "${PCRE_LIBRARY_RELEASE}")
    endif()
    if(PCRE_LIBRARY_DEBUG)
      set_target_properties(PCRE::PCRE PROPERTIES
                                       IMPORTED_CONFIGURATIONS DEBUG
                                       IMPORTED_LOCATION "${PCRE_LIBRARY_DEBUG}")
    endif()
    set_target_properties(PCRE::PCRE PROPERTIES
                                     INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
    if(WIN32)
      set_target_properties(PCRE::PCRE PROPERTIES
                                       INTERFACE_COMPILE_DEFINITIONS PCRE_STATIC=1)
    endif()

  endif()
  if(NOT TARGET PCRE::PCRECPP)
    add_library(PCRE::PCRECPP UNKNOWN IMPORTED)
    if(PCRE_LIBRARY_RELEASE)
      set_target_properties(PCRE::PCRECPP PROPERTIES
                                          IMPORTED_CONFIGURATIONS RELEASE
                                          IMPORTED_LOCATION "${PCRECPP_LIBRARY_RELEASE}")
    endif()
    if(PCRE_LIBRARY_DEBUG)
      set_target_properties(PCRE::PCRECPP PROPERTIES
                                          IMPORTED_CONFIGURATIONS DEBUG
                                          IMPORTED_LOCATION "${PCRECPP_LIBRARY_DEBUG}")
    endif()
    set_target_properties(PCRE::PCRECPP PROPERTIES
                                        INTERFACE_LINK_LIBRARIES PCRE::PCRE)
  endif()
endif()

mark_as_advanced(PCRE_INCLUDE_DIR PCRECPP_LIBRARY PCRE_LIBRARY)
