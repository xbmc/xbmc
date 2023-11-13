#.rst:
# FindCEC
# -------
# Finds the libCEC library
#
# This will define the following target:
#
#   CEC::CEC - The libCEC library

if(NOT TARGET CEC::CEC)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildCEC)
    set(CEC_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-all-cmakelists.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/002-all-libceccmakelists.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/003-all-remove_git_info.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/004-win-remove_32bit_timet.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/005-win-pdbstatic.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=ON
                   -DSKIP_PYTHON_WRAPPER=ON
                   -DCMAKE_PLATFORM_NO_VERSIONED_SONAME=ON)

    if(WIN32 AND ARCH STREQUAL "x64")
      # Disable _USE_32BIT_TIME_T for x64 win target
      list(APPEND CMAKE_ARGS -DWIN64=ON)
    endif()

    if(CORE_SYSTEM_NAME STREQUAL "osx")
      set(CEC_BYPRODUCT_EXTENSION "dylib")
    endif()

    BUILD_DEP_TARGET()

    if(CORE_SYSTEM_NAME STREQUAL "osx")
      find_program(INSTALL_NAME_TOOL NAMES install_name_tool)
      add_custom_command(TARGET cec POST_BUILD
                                    COMMAND ${INSTALL_NAME_TOOL} -id ${CEC_LIBRARY} ${CEC_LIBRARY})
    endif()

    add_dependencies(cec P8Platform::P8Platform)
  endmacro()

  # We only need to check p8-platform if we have any intention to build internal
  if(ENABLE_INTERNAL_CEC)
    # Check for dependencies - Must be done before SETUP_BUILD_VARS
    get_libversion_data("p8-platform" "target")
    find_package(P8Platform ${LIB_P8-PLATFORM_VER} MODULE QUIET REQUIRED)
    # Check if we want to force a build due to a dependency rebuild
    get_property(LIB_FORCE_REBUILD TARGET P8Platform::P8Platform PROPERTY LIB_BUILD)
  endif()

  set(MODULE_LC cec)

  SETUP_BUILD_VARS()

  # Check for existing libcec. If version >= LIBCEC-VERSION file version, dont build
  find_package(libcec CONFIG
                      HINTS ${DEPENDS_PATH}/lib/cmake
                      ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  if((libcec_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_CEC) OR LIB_FORCE_REBUILD)
    # Build lib
    buildCEC()
  else()
    # if libcec::cec target exists, it meets version requirements
    # we only do a pkgconfig search when a suitable cmake config returns nothing
    if(TARGET libcec::cec)
      get_target_property(_CEC_CONFIGURATIONS libcec::cec IMPORTED_CONFIGURATIONS)
      foreach(_cec_config IN LISTS _CEC_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_cec_config} _cec_config_UPPER)
        if((NOT ${_cec_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_cec_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(CEC_LIBRARY_RELEASE libcec::cec IMPORTED_LOCATION_${_cec_config_UPPER})
        else()
          get_target_property(CEC_LIBRARY_${_cec_config_UPPER} libcec::cec IMPORTED_LOCATION_${_cec_config_UPPER})
        endif()
      endforeach()

      # CEC cmake config doesnt include INTERFACE_INCLUDE_DIRECTORIES
      find_path(CEC_INCLUDE_DIR NAMES libcec/cec.h libCEC/CEC.h
                                HINTS ${DEPENDS_PATH}/include ${PC_CEC_INCLUDEDIR}
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                NO_CACHE)
      set(CEC_VERSION ${libcec_VERSION})
    else()
      find_package(PkgConfig)
      # Fallback to pkg-config and individual lib/include file search
      if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_CEC libcec QUIET)
      endif()
      find_library(CEC_LIBRARY_RELEASE NAMES cec
                                       HINTS ${DEPENDS_PATH}/lib ${PC_CEC_LIBDIR}
                                       ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                       NO_CACHE)

      find_path(CEC_INCLUDE_DIR NAMES libcec/cec.h libCEC/CEC.h
                                HINTS ${DEPENDS_PATH}/include ${PC_CEC_INCLUDEDIR}
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                NO_CACHE)

      if(PC_CEC_VERSION)
        set(CEC_VERSION ${PC_CEC_VERSION})
      elseif(CEC_INCLUDE_DIR AND EXISTS "${CEC_INCLUDE_DIR}/libcec/version.h")
        file(STRINGS "${CEC_INCLUDE_DIR}/libcec/version.h" cec_version_str REGEX "^[\t ]+LIBCEC_VERSION_TO_UINT\\(.*\\)")
        string(REGEX REPLACE "^[\t ]+LIBCEC_VERSION_TO_UINT\\(([0-9]+), ([0-9]+), ([0-9]+)\\)" "\\1.\\2.\\3" CEC_VERSION "${cec_version_str}")
        unset(cec_version_str)
      endif()
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(CEC)
  unset(CEC_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CEC
                                    REQUIRED_VARS CEC_LIBRARY CEC_INCLUDE_DIR
                                    VERSION_VAR CEC_VERSION)

  if(CEC_FOUND)
    # cmake target and not building internal
    if(TARGET libcec::cec AND NOT TARGET cec)
      add_library(CEC::CEC ALIAS libcec::cec)
      # We need to append in case the cmake config already has definitions
      set_property(TARGET libcec::cec APPEND PROPERTY
                                      INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCEC=1)
    # pkgconfig target found
    elseif(TARGET PkgConfig::PC_CEC)
      add_library(CEC::CEC ALIAS PkgConfig::PC_CEC)
      set_property(TARGET PkgConfig::PC_CEC APPEND PROPERTY
                                            INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCEC=1)
    # building internal or no cmake config or pkgconfig
    else()
      add_library(CEC::CEC UNKNOWN IMPORTED)
      set_target_properties(CEC::CEC PROPERTIES
                                     IMPORTED_LOCATION "${CEC_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${CEC_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBCEC=1)
    endif()

    if(TARGET cec)
      add_dependencies(CEC::CEC cec)
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
      if(NOT TARGET cec)
        buildCEC()
        set_target_properties(cec PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends cec)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CEC::CEC)

  endif()
endif()
