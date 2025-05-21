# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Fmt   - The Fmt library
#   LIBRARY::Fmt   - An ALIAS TARGET for the Fmt Library if built internally
#                    This is due to the ability of the Fmt Lib to be a dependency
#                    of other libs

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroFmt)
    if(APPLE)
      set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    # fmt debug uses postfix d for all platforms
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-windows-pdb-symbol-gen.patch")
      generate_patchcommand("${patches}")
    endif()

    set(CMAKE_ARGS -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DFMT_DOC=OFF
                   -DFMT_TEST=OFF
                   -DFMT_INSTALL=ON
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC fmt)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_FMT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FMT))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET fmt::fmt)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is fmtConfigTargets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_FMT_CONFIGURATIONS fmt::fmt IMPORTED_CONFIGURATIONS)
      if(_FMT_CONFIGURATIONS)
        foreach(_fmt_config IN LISTS _FMT_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_fmt_config} _fmt_config_UPPER)
          if((NOT ${_fmt_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_fmt_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE fmt::fmt IMPORTED_LOCATION_${_fmt_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_fmt_config_UPPER} fmt::fmt IMPORTED_LOCATION_${_fmt_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE fmt::fmt IMPORTED_LOCATION)
      endif()

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)
  if(Fmt_FOUND)
    if(TARGET fmt::fmt AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS fmt::fmt)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      # Add ALIAS TARGET as Fmt can be a dependency of other libs
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Fmt_FIND_REQUIRED)
      message(FATAL_ERROR "Fmt libraries were not found. You may want to use -DENABLE_INTERNAL_FMT=ON")
    endif()
  endif()
endif()
