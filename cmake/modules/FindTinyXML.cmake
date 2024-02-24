#.rst:
# FindTinyXML
# -----------
# Finds the TinyXML library
#
# This will define the following variables::
#
# TINYXML_FOUND - system has TinyXML
# TINYXML_INCLUDE_DIRS - the TinyXML include directory
# TINYXML_LIBRARIES - the TinyXML libraries
# TINYXML_DEFINITIONS - the TinyXML definitions
#
# and the following imported targets::
#
#   TinyXML::TinyXML   - The TinyXML library

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_TINYXML tinyxml QUIET)
endif()

find_path(TINYXML_INCLUDE_DIR tinyxml.h
                              PATH_SUFFIXES tinyxml
                              HINTS ${PC_TINYXML_INCLUDEDIR})
find_library(TINYXML_LIBRARY_RELEASE NAMES tinyxml tinyxmlSTL
                                     PATH_SUFFIXES tinyxml
                                     HINTS ${PC_TINYXML_LIBDIR})
find_library(TINYXML_LIBRARY_DEBUG NAMES tinyxmld tinyxmlSTLd
                                   PATH_SUFFIXES tinyxml
                                   HINTS ${PC_TINYXML_LIBDIR})
set(TINYXML_VERSION ${PC_TINYXML_VERSION})

include(SelectLibraryConfigurations)
select_library_configurations(TINYXML)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TinyXML
                                  REQUIRED_VARS TINYXML_LIBRARY TINYXML_INCLUDE_DIR
                                  VERSION_VAR TINYXML_VERSION)

if(TINYXML_FOUND)
  set(TINYXML_LIBRARIES ${TINYXML_LIBRARY})
  set(TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDE_DIR})
  if(WIN32)
    set(TINYXML_DEFINITIONS -DTIXML_USE_STL=1)
  endif()

  if(NOT TARGET TinyXML::TinyXML)
    add_library(TinyXML::TinyXML UNKNOWN IMPORTED)
    if(TINYXML_LIBRARY_RELEASE)
      set_target_properties(TinyXML::TinyXML PROPERTIES
                                             IMPORTED_CONFIGURATIONS RELEASE
                                             IMPORTED_LOCATION "${TINYXML_LIBRARY_RELEASE}")
    endif()
    if(TINYXML_LIBRARY_DEBUG)
      set_target_properties(TinyXML::TinyXML PROPERTIES
                                             IMPORTED_CONFIGURATIONS DEBUG
                                             IMPORTED_LOCATION "${TINYXML_LIBRARY_DEBUG}")
    endif()
    set_target_properties(TinyXML::TinyXML PROPERTIES
                                           INTERFACE_INCLUDE_DIRECTORIES "${TINYXML_INCLUDE_DIR}")
    if(WIN32)
      set_target_properties(TinyXML::TinyXML PROPERTIES
                                             INTERFACE_COMPILE_DEFINITIONS TIXML_USE_STL=1)
    endif()
  endif()
endif()

mark_as_advanced(TINYXML_INCLUDE_DIR TINYXML_LIBRARY)
