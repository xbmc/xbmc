#.rst:
# FindFriBidi
# -----------
# Finds the GNU FriBidi library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FriBidi   - The FriBidi library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC fribidi)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET fribidi::fribidi)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS fribidi::fribidi)
    endif()

    get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)
    add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_ALIASTARGET})

    # Common TARGET name other libs use
    if(NOT TARGET FriBidi::FriBidi)
      add_library(FriBidi::FriBidi ALIAS ${_ALIASTARGET})
    endif()
  else()
    if(FriBidi_FIND_REQUIRED)
      message(FATAL_ERROR "FriBidi library was not found.")
    endif()
  endif()
endif()
