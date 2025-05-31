#.rst:
# FindFreetype
# ------------
# Finds the FreeType library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FreeType   - The FreeType library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC freetype2)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET freetype::freetype)
      # Kodi target - windows prebuilt lib
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS freetype::freetype)
    elseif(TARGET Freetype::Freetype)
      # Freetype native target
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Freetype::Freetype)
    endif()

    if(NOT TARGET Freetype::Freetype)
      get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)

      add_library(Freetype::Freetype ALIAS ${_ALIASTARGET})
    endif()
  else()
    if(Freetype_FIND_REQUIRED)
      message(FATAL_ERROR "Freetype libraries were not found.")
    endif()
  endif()
endif()
