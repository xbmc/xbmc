#.rst:
# FindASS
# -------
# Finds the ASS library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::ASS   - The ASS library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig)
  # Do not use pkgconfig on windows
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_ASS libass QUIET IMPORTED_TARGET)

    # INTERFACE_LINK_OPTIONS is incorrectly populated when cmake generation is executed
    # when an existing build generation is already done. Just set this to blank
    set_target_properties(PkgConfig::PC_ASS PROPERTIES INTERFACE_LINK_OPTIONS "")

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET PC_ASS_LINK_LIBRARIES 0 ASS_LIBRARY)
    set(ASS_INCLUDE_DIR ${PC_ASS_INCLUDEDIR})
    set(ASS_VERSION ${PC_ASS_VERSION})
  elseif(WIN32 OR WINDOWS_STORE)
    find_package(libass CONFIG QUIET REQUIRED
                        HINTS ${DEPENDS_PATH}/lib/cmake
                        ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

    # we only do this because we use find_package_handle_standard_args for config time output
    # and it isnt capable of handling TARGETS, so we have to extract the info
    get_target_property(_ASS_CONFIGURATIONS libass::libass IMPORTED_CONFIGURATIONS)
    foreach(_ass_config IN LISTS _ASS_CONFIGURATIONS)
      # Some non standard config (eg None on Debian)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_ass_config} _ass_config_UPPER)
      if((NOT ${_ass_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_ass_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(ASS_LIBRARY_RELEASE libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
      else()
        get_target_property(ASS_LIBRARY_${_ass_config_UPPER} libass::libass IMPORTED_LOCATION_${_ass_config_UPPER})
      endif()
    endforeach()

    get_target_property(ASS_INCLUDE_DIR libass::libass INTERFACE_INCLUDE_DIRECTORIES)
    set(ASS_VERSION ${libass_VERSION})

    include(SelectLibraryConfigurations)
    select_library_configurations(ASS)
    unset(ASS_LIBRARIES)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ASS
                                    REQUIRED_VARS ASS_LIBRARY ASS_INCLUDE_DIR
                                    VERSION_VAR ASS_VERSION)

  if(ASS_FOUND)
    if(TARGET PkgConfig::PC_ASS)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::PC_ASS)
    elseif(TARGET libass::libass)
      # Kodi custom libass target used for windows platforms
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libass::libass)
    endif()
  else()
    if(ASS_FIND_REQUIRED)
      message(FATAL_ERROR "Ass libraries were not found.")
    endif()
  endif()
endif()
