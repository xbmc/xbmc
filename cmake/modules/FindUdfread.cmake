#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Udfread - The libudfread library
#   LIBRARY::Udfread - ALIAS target for the libudfread library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC udfread)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME libudfread)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # windows cmake config populated target
    if(TARGET libudfread::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_UDFREAD)
    ADD_TARGET_COMPILE_DEFINITION()
  else()
    if(Udfread_FIND_REQUIRED)
      message(FATAL_ERROR "Udfread libraries were not found.")
    endif()
  endif()
endif()
