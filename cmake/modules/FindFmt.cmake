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

  # Macro for building INTERNAL_FMT
  macro(buildFmt)
    if(APPLE)
      set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
    endif()

    set(FMT_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
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

  # Check for existing FMT. If version >= FMT-VERSION file version, dont build
  find_package(FMT ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                   HINTS ${DEPENDS_PATH}/lib/cmake
                   ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available
  # fallback to pkgconfig for non windows platforms
  if(NOT FMT_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(FMT fmt${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if((FMT_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_FMT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FMT))
    # build internal module
    buildFmt()
  else()
    if(TARGET fmt::fmt)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is fmtConfigTargets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_FMT_CONFIGURATIONS fmt::fmt IMPORTED_CONFIGURATIONS)
      foreach(_fmt_config IN LISTS _FMT_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_fmt_config} _fmt_config_UPPER)
        if((NOT ${_fmt_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_fmt_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(FMT_LIBRARY_RELEASE fmt::fmt IMPORTED_LOCATION_${_fmt_config_UPPER})
        else()
          get_target_property(FMT_LIBRARY_${_fmt_config_UPPER} fmt::fmt IMPORTED_LOCATION_${_fmt_config_UPPER})
        endif()
      endforeach()

      get_target_property(FMT_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::FMT)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET FMT_LINK_LIBRARIES 0 FMT_LIBRARY_RELEASE)

      get_target_property(FMT_INCLUDE_DIR PkgConfig::FMT INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(FMT)
  unset(FMT_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
                                    VERSION_VAR FMT_VERSION)
  if(Fmt_FOUND)
    if(TARGET fmt::fmt AND NOT TARGET fmt)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS fmt::fmt)
    elseif(TARGET PkgConfig::FMT AND NOT TARGET fmt)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::FMT)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")

      if(FMT_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${FMT_LIBRARY_RELEASE}")
      endif()
      if(FMT_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${FMT_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()
    endif()

    if(TARGET fmt)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} fmt)

      # Add ALIAS TARGET as Fmt can be a dependency of other libs
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET fmt)
        buildFmt()
        set_target_properties(fmt PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends fmt)
    endif()
  else()
    if(Fmt_FIND_REQUIRED)
      message(FATAL_ERROR "Fmt libraries were not found. You may want to use -DENABLE_INTERNAL_FMT=ON")
    endif()
  endif()
endif()
