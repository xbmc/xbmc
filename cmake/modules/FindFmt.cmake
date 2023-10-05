# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following target:
#
#   fmt::fmt   - The Fmt library

if(NOT TARGET fmt::fmt)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Macro for building INTERNAL_FMT
  macro(buildFmt)
    if(APPLE)
      set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
    endif()

    set(FMT_VERSION ${${MODULE}_VER})
    # fmt debug uses postfix d for all platforms
    set(FMT_DEBUG_POSTFIX d)

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-windows-pdb-symbol-gen.patch")
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

  set(MODULE_LC fmt)
  # Default state
  set(FORCE_BUILD OFF)

  SETUP_BUILD_VARS()

  # Check for existing FMT. If version >= FMT-VERSION file version, dont build
  find_package(FMT CONFIG QUIET
                   HINTS ${DEPENDS_PATH}/lib/cmake
                   ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})


  if((FMT_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_FMT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FMT))

    # Set FORCE_BUILD to enable fmt::fmt property that build will occur
    set(FORCE_BUILD ON)
    buildFmt()
  else()
    if(NOT TARGET fmt::fmt)
      # Do not use pkgconfig on windows
      if(PKG_CONFIG_FOUND AND NOT WIN32)
        pkg_check_modules(PC_FMT libfmt QUIET)
        set(FMT_VERSION ${PC_FMT_VERSION})
      endif()

      find_path(FMT_INCLUDE_DIR NAMES fmt/format.h
                                HINTS ${DEPENDS_PATH}/include ${PC_FMT_INCLUDEDIR}
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                NO_CACHE)

      find_library(FMT_LIBRARY_RELEASE NAMES fmt
                                       HINTS ${DEPENDS_PATH}/lib ${PC_FMT_LIBDIR}
                                       ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                       NO_CACHE)
      find_library(FMT_LIBRARY_DEBUG NAMES fmtd
                                     HINTS ${DEPENDS_PATH}/lib ${PC_FMT_LIBDIR}
                                     ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                     NO_CACHE)
    endif()
  endif()

  # fmt::fmt target exists and is of suitable versioning. the INTERNAL_FMT build
  # is not created.
  # We create variables based off TARGET data for use with FPHSA
  if(TARGET fmt::fmt AND NOT TARGET fmt)
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
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(FMT)
  unset(FMT_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
                                    VERSION_VAR FMT_VERSION)
  if(Fmt_FOUND)
    if(TARGET fmt OR NOT TARGET fmt::fmt)
      if(NOT TARGET fmt::fmt)
        add_library(fmt::fmt UNKNOWN IMPORTED)
      endif()
      if(FMT_LIBRARY_RELEASE)
        set_target_properties(fmt::fmt PROPERTIES
                                       IMPORTED_CONFIGURATIONS RELEASE
                                       IMPORTED_LOCATION_RELEASE "${FMT_LIBRARY_RELEASE}")
      endif()
      if(FMT_LIBRARY_DEBUG)
        set_target_properties(fmt::fmt PROPERTIES
                                       IMPORTED_CONFIGURATIONS DEBUG
                                       IMPORTED_LOCATION_DEBUG "${FMT_LIBRARY_DEBUG}")
      endif()
      set_target_properties(fmt::fmt PROPERTIES
                                     INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
    endif()

    if(TARGET fmt)
      add_dependencies(fmt::fmt fmt)
      # If a force build is done, let any calling packages know they may want to rebuild
      if(FORCE_BUILD)
        set_target_properties(fmt::fmt PROPERTIES LIB_BUILD ON)
      endif()
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
  endif()

  # Check whether we already have fmt::fmt target added to dep property list
  get_property(CHECK_INTERNAL_DEPS GLOBAL PROPERTY INTERNAL_DEPS_PROP)
  list(FIND CHECK_INTERNAL_DEPS "fmt::fmt" FMT_PROP_FOUND)

  # list(FIND) returns -1 if search item not found
  if(FMT_PROP_FOUND STREQUAL "-1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP fmt::fmt)
  endif()
endif()
