#.rst:
# FindGObject
# --------
# Finds the GObject library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::GObject   - The GObject library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GOBJECT gobject-2.0 ${SEARCH_QUIET})
  endif()

  find_path(GOBJECT_INCLUDE_DIR NAMES glib-object.h
                                HINTS ${PC_GOBJECT_INCLUDEDIR}
                                PATH_SUFFIXES glib-2.0)
  find_path(GLIBCONFIG_INCLUDE_DIR NAMES glibconfig.h
                                   HINTS ${PC_GOBJECT_INCLUDEDIR} ${CMAKE_FIND_ROOT_PATH}
                                   PATH_SUFFIXES lib/glib-2.0/include)

  find_library(GOBJECT_LIBRARY NAMES gobject-2.0
                               HINTS ${PC_GOBJECT_LIBDIR})

  set(GOBJECT_VERSION ${PC_GOBJECT_VERSION})

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GObject
                                    REQUIRED_VARS GOBJECT_LIBRARY GOBJECT_INCLUDE_DIR GLIBCONFIG_INCLUDE_DIR
                                    VERSION_VAR GOBJECT_VERSION)

  if(GOBJECT_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${GOBJECT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${GOBJECT_INCLUDE_DIR};${GLIBCONFIG_INCLUDE_DIR}")
  else()
    if(GObject_FIND_REQUIRED)
      message(FATAL_ERROR "GObject library not found.")
    endif()
  endif()
endif()
