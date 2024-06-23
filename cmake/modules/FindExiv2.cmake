#.rst:
# FindExiv2
# -------
# Finds the exiv2 library
#
# This will define the following imported targets::
#
#   ${APP_NAME_LC}::Exiv2   - The EXIV2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildexiv2)

    find_package(Patch MODULE REQUIRED)
    find_package(Iconv REQUIRED)

    # Note: Please drop once a release based on master is made. First 2 are already upstream.
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Add-EXIV2_ENABLE_FILESYSTEM_ACCESS-option.patch"
    "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Set-conditional-HTTP-depending-on-EXIV2_ENABLE_WEBRE.patch"
    "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0003-UWP-Disable-getLoadedLibraries.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DEXIV2_ENABLE_WEBREADY=OFF
                   -DEXIV2_ENABLE_XMP=OFF
                   -DEXIV2_ENABLE_CURL=OFF
                   -DEXIV2_ENABLE_NLS=OFF
                   -DEXIV2_BUILD_SAMPLES=OFF
                   -DEXIV2_BUILD_UNIT_TESTS=OFF
                   -DEXIV2_ENABLE_VIDEO=OFF
                   -DEXIV2_ENABLE_BMFF=OFF
                   -DEXIV2_ENABLE_BROTLI=OFF
                   -DEXIV2_ENABLE_INIH=OFF
                   -DEXIV2_ENABLE_FILESYSTEM_ACCESS=OFF
                   -DEXIV2_BUILD_EXIV2_COMMAND=OFF)

    if(NOT CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "")
      list(APPEND CMAKE_ARGS -DBUILD_WITH_CCACHE=ON)
    endif()

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC exiv2)

  SETUP_BUILD_VARS()

  find_package(exiv2 CONFIG QUIET
                            HINTS ${DEPENDS_PATH}/lib/cmake
                            ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing EXIV2. If version >= EXIV2-VERSION file version, dont build
  if((exiv2_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_EXIV2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_EXIV2))

    buildexiv2()
  else()
    if(NOT TARGET Exiv2::exiv2lib)
      find_package(PkgConfig)
      # Fallback to pkg-config and individual lib/include file search
      if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_EXIV2 exiv2 QUIET)
        set(EXIV2_VER ${PC_EXIV2_VERSION})
      endif()

      find_path(EXIV2_INCLUDE_DIR NAMES exiv2/exiv2.hpp
                                  HINTS ${PC_EXIV2_INCLUDEDIR} NO_CACHE)
      find_library(EXIV2_LIBRARY NAMES exiv2
                                 HINTS ${PC_EXIV2_LIBDIR} NO_CACHE)
    endif()
  endif()

  # We create variables based off TARGET data for use with FPHSA
  if(TARGET Exiv2::exiv2lib AND NOT TARGET exiv2)
    # This is for the case where a distro provides a non standard (Debug/Release) config type
    # eg Debian's config file is exiv2Config-none.cmake
    # convert this back to either DEBUG/RELEASE or just RELEASE
    # we only do this because we use find_package_handle_standard_args for config time output
    # and it isnt capable of handling TARGETS, so we have to extract the info
    get_target_property(_FMT_CONFIGURATIONS Exiv2::exiv2lib IMPORTED_CONFIGURATIONS)
    foreach(_exiv2_config IN LISTS _FMT_CONFIGURATIONS)
      # Some non standard config (eg None on Debian)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_exiv2_config} _exiv2_config_UPPER)
      if((NOT ${_exiv2_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_exiv2_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(EXIV2_LIBRARY_RELEASE Exiv2::exiv2lib IMPORTED_LOCATION_${_exiv2_config_UPPER})
      else()
        get_target_property(EXIV2_LIBRARY_${_exiv2_config_UPPER} Exiv2::exiv2lib IMPORTED_LOCATION_${_exiv2_config_UPPER})
      endif()
    endforeach()

    get_target_property(EXIV2_INCLUDE_DIR Exiv2::exiv2lib INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(EXIV2)
  unset(EXIV2_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Exiv2
                                    REQUIRED_VARS EXIV2_LIBRARY EXIV2_INCLUDE_DIR
                                    VERSION_VAR EXIV2_VER)

  if(EXIV2_FOUND)
    if(TARGET Exiv2::exiv2lib AND NOT TARGET exiv2)
      # Exiv2 config found. Use it
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Exiv2::exiv2lib)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${EXIV2_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${EXIV2_INCLUDE_DIR}")
      if(CORE_SYSTEM_NAME STREQUAL "freebsd")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_LINK_LIBRARIES procstat)
      elseif(CORE_SYSTEM_NAME MATCHES "windows")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_LINK_LIBRARIES psapi)
      endif()
    endif()

    if(TARGET exiv2)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} exiv2)
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
      if(NOT TARGET exiv2)
        buildexiv2()
        set_target_properties(exiv2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends exiv2)
    endif()
  else()
    if(Exiv2_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Exiv2 library. You may want to try -DENABLE_INTERNAL_EXIV2=ON")
    endif()
  endif()
endif()
