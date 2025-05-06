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

  if(TARGET libudfread::libudfread)
    get_target_property(UDFREAD_LIBRARY libudfread::libudfread IMPORTED_LOCATION_RELWITHDEBINFO)
    get_target_property(UDFREAD_INCLUDE_DIR libudfread::libudfread INTERFACE_INCLUDE_DIRECTORIES)
  elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY)

    get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  find_package_handle_standard_args(Udfread
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Udfread_FOUND)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_UDFREAD)

    # windows cmake config populated target
    if(TARGET libudfread::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
    # pkgconfig populated target that is sufficient version
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()

    ADD_TARGET_COMPILE_DEFINITION()
  else()
    if(Udfread_FIND_REQUIRED)
      message(FATAL_ERROR "Udfread libraries were not found.")
    endif()
  endif()
endif()
