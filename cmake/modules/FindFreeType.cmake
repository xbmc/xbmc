#.rst:
# FindFreetype
# ------------
# Finds the FreeType library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FreeType   - The FreeType library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  # Do not use pkgconfig on windows
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_FREETYPE freetype2 ${SEARCH_QUIET})
  endif()

  find_path(FREETYPE_INCLUDE_DIR NAMES freetype/freetype.h freetype.h
                                 HINTS ${DEPENDS_PATH}/include
                                       ${PC_FREETYPE_INCLUDEDIR}
                                       ${PC_FREETYPE_INCLUDE_DIRS}
                                 PATH_SUFFIXES freetype2
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  find_library(FREETYPE_LIBRARY NAMES freetype freetype246MT
                                HINTS ${DEPENDS_PATH}/lib ${PC_FREETYPE_LIBDIR}
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  set(FREETYPE_VERSION ${PC_FREETYPE_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FreeType
                                    REQUIRED_VARS FREETYPE_LIBRARY FREETYPE_INCLUDE_DIR
                                    VERSION_VAR FREETYPE_VERSION)

  if(FREETYPE_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}")

    if(NOT TARGET Freetype::Freetype)
      add_library(Freetype::Freetype ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    endif()
  else()
    if(Freetype_FIND_REQUIRED)
      message(FATAL_ERROR "Freetype libraries were not found.")
    endif()
  endif()
endif()
