if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GSTREAMER gstreamer-1.0 ${SEARCH_QUIET})
  endif()

  find_path(GSTREAMER_INCLUDE_DIR NAMES gst/gst.h
                                  HINTS ${PC_GSTREAMER_INCLUDEDIR}
                                  PATH_SUFFIXES gstreamer-1.0)

  find_library(GSTREAMER_LIBRARY NAMES gstreamer-1.0
                                 HINTS ${PC_GSTREAMER_LIBDIR})

  set(GSTREAMER_VERSION ${PC_GSTREAMER_VERSION})

  if(NOT VERBOSE_FIND)
    set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GStreamer
                                    REQUIRED_VARS GSTREAMER_LIBRARY GSTREAMER_INCLUDE_DIR
                                    VERSION_VAR GSTREAMER_VERSION)

  if(GSTREAMER_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${GSTREAMER_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE_DIR}")
  else()
    if(GStreamer_FIND_REQUIRED)
      message(FATAL_ERROR "GStreamer library not found.")
    endif()
  endif()
endif()
