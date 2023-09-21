# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following target:
#
#   fmt::fmt   - The Fmt library

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

define_property(TARGET PROPERTY LIB_BUILD
                       BRIEF_DOCS "This target will be compiling the library"
                       FULL_DOCS "This target will be compiling the library")

set(FORCE_BUILD OFF)

# If target exists, no need to rerun find
# Allows a module that may be a dependency for multiple libraries to just be executed
# once to populate all required variables/targets
if(NOT TARGET fmt::fmt OR Fmt_FIND_REQUIRED)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC fmt)

  SETUP_BUILD_VARS()

  # Check for existing FMT. If version >= FMT-VERSION file version, dont build
  find_package(FMT CONFIG QUIET
                   HINTS ${DEPENDS_PATH}/lib
                   ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Build if ENABLE_INTERNAL_FMT, or if required version in find_package call is greater 
  # than already found FMT_VERSION from a previous find_package call
  if((Fmt_FIND_REQUIRED AND FMT_VERSION VERSION_LESS Fmt_FIND_VERSION AND ENABLE_INTERNAL_FMT) OR
     (FMT_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_FMT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FMT))

    if(Fmt_FIND_VERSION)
      if(FMT_VERSION VERSION_LESS ${Fmt_FIND_VERSION})
        set(FORCE_BUILD ON)
      endif()
    endif()

    if(${FORCE_BUILD} OR FMT_VERSION VERSION_LESS ${${MODULE}_VER})

      # Set FORCE_BUILD to enable fmt::fmt property that build will occur
      set(FORCE_BUILD ON)

      buildFmt()

    else()
      if(NOT TARGET fmt::fmt)
        set(FMT_PKGCONFIG_CHECK ON)
      endif()
    endif()
  else()
    if(NOT TARGET fmt::fmt)
      set(FMT_PKGCONFIG_CHECK ON)
    endif()
  endif()

  if(NOT TARGET fmt::fmt)
    if(FMT_PKGCONFIG_CHECK)
      if(PKG_CONFIG_FOUND)
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

    add_library(fmt::fmt UNKNOWN IMPORTED)
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

    if(TARGET fmt)
      add_dependencies(fmt::fmt fmt)
    else()
      # Add internal build target when a Multi Config Generator is used
      # We cant add a dependency based off a generator expression for targeted build types,
      # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
      # therefore if the find heuristics only find the library, we add the internal build 
      # target to the project to allow user to manually trigger for any build type they need
      # in case only a specific build type is actually available (eg Release found, Debug Required)
      # This is mainly targeted for windows who required different runtime libs for different
      # types, and they arent compatible
      if(_multiconfig_generator)
        buildFmt()
      endif()
    endif()
  endif()

  # If a force build is done, let any calling packages know they may want to rebuild
  if(FORCE_BUILD)
    set_target_properties(fmt::fmt PROPERTIES LIB_BUILD ON)
  endif()

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

  include(SelectLibraryConfigurations)
  select_library_configurations(FMT)
  unset(FMT_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
                                    VERSION_VAR FMT_VERSION)

  # Check whether we already have fmt::fmt target added to dep property list
  get_property(CHECK_INTERNAL_DEPS GLOBAL PROPERTY INTERNAL_DEPS_PROP)
  list(FIND CHECK_INTERNAL_DEPS "fmt::fmt" FMT_PROP_FOUND)

  # list(FIND) returns -1 if search item not found
  if(FMT_PROP_FOUND STREQUAL "-1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP fmt::fmt)
  endif()

endif()
