#.rst:
# FindTinyXML
# -----------
# Finds the TinyXML library
#
# The following imported targets are created::
#
#   ${APP_NAME_LC}::TinyXML   - The TinyXML library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_TINYXML tinyxml ${SEARCH_QUIET})
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
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(TINYXML_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${TINYXML_LIBRARY_RELEASE}")
    endif()
    if(TINYXML_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${TINYXML_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${TINYXML_INCLUDE_DIR}")
    if(WIN32 OR WINDOWS_STORE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS TIXML_USE_STL)
    endif()
  else()
    if(TinyXML_FIND_REQUIRED)
      message(FATAL_ERROR "TinyXML library not found.")
    endif()
  endif()
endif()
