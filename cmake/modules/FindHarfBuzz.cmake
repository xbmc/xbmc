#.rst:
# FindHarfbuzz
# ------------
# Finds the HarfBuzz library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::HarfBuzz   - The HarfBuzz library
#   LIBRARY::HarfBuzz   - ALIAS TARGET for the HarfBuzz library

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

      if(WIN32 OR WINDOWS_STORE)
        # Harfbuzz cmake config has a full path reference to freetype.
        # As we currently use a prebuilt harfbuzz, this ends up with the path to freetype being the path
        # used on the GH Action that builds the prebuilt libs. remove and add TARGET for freetype
        get_target_property(_link_libs harfbuzz::harfbuzz INTERFACE_LINK_LIBRARIES)
        list(REMOVE_AT _link_libs 0)
        list(INSERT _link_libs 0 "freetype::freetype")
        set_target_properties(harfbuzz::harfbuzz PROPERTIES INTERFACE_LINK_LIBRARIES "${_link_libs}")
      endif()
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()

    get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)
    add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_ALIASTARGET})

    if(NOT TARGET HarfBuzz::HarfBuzz)
      add_library(HarfBuzz::HarfBuzz ALIAS ${_ALIASTARGET})
    endif()
  else()
    if(HarfBuzz_FIND_REQUIRED)
      message(FATAL_ERROR "Harfbuzz libraries were not found.")
    endif()
  endif()
endif()
