#.rst:
# FindHarfbuzz
# ------------
# Finds the HarfBuzz library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::HarfBuzz   - The HarfBuzz library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC harfbuzz)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET harfbuzz::harfbuzz)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS harfbuzz::harfbuzz)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()

    if(NOT TARGET HarfBuzz::HarfBuzz)
      get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)

      add_library(HarfBuzz::HarfBuzz ALIAS ${_ALIASTARGET})
    endif()
  else()
    if(HarfBuzz_FIND_REQUIRED)
      message(FATAL_ERROR "Harfbuzz libraries were not found.")
    endif()
  endif()
endif()
