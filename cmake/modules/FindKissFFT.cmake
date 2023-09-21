#.rst:
# FindKissFFT
# ------------
# Finds the KissFFT as a Fast Fourier Transformation (FFT) library
#
# This will define the following target:
#
#   kissfft::libkissfft   - The KissFFT library

if(NOT TARGET kissfft::libkissfft)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Build lib macro
  macro(buildKissFFT)
    set(KISSFFT_VERSION ${${MODULE}_VER})

    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-win-debugpostfix.patch")
      generate_patchcommand("${patches}")
    endif()

    # manual build name required as kissfft cmake config can generate a target with the name kissfft
    set(BUILD_NAME build-kissfft)

    set(CMAKE_ARGS -DKISSFFT_STATIC=ON
                   -DKISSFFT_TOOLS=OFF
                   -DKISSFFT_PKGCONFIG=OFF
                   -DCMAKE_INSTALL_LIBDIR=${DEPENDS_PATH}/lib
                   -DCMAKE_INSTALL_INCLUDEDIR=${DEPENDS_PATH}/include
                   -DKISSFFT_TEST=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  set(MODULE_LC kissfft)

  SETUP_BUILD_VARS()

  # internal build we always expect STATIC
  if(ENABLE_INTERNAL_KISSFFT OR KODI_DEPENDSBUILD)
    set(KISSFFT_COMPONENT_SEARCH COMPONENTS STATIC)
  endif()

  # Check for existing kissfft
  find_package(kissfft CONFIG QUIET
                              ${KISSFFT_COMPONENT_SEARCH}
                              HINTS ${DEPENDS_PATH}/lib/cmake
                              ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing kissfft. If version >= KISSFFT-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal, build anyway
  if((kissfft_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_KISSFFT) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_KISSFFT))
    # Call build macro
    buildKissFFT()
  else()
    if(NOT TARGET kissfft::kissfft)
      find_package(PkgConfig)
      # Do not use pkgconfig on windows
      if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
        # kissfft has multiple possible libs.
        # search from most common (kissfft-float), and then highest to lowest precision
        set(_kissfft_variants kissfft-float kissfft-double kissfft-int32 kissfft-int16)

        foreach(_variant ${_kissfft_variants})
          pkg_check_modules(KISSFFT QUIET ${_variant})

          if(KISSFFT_FOUND)
            # First item is the full path of the library file found
            # pkg_check_modules does not populate a variable of the found library explicitly
            list(GET KISSFFT_LINK_LIBRARIES 0 KISSFFT_LIBRARY_RELEASE)
            set(KISSFFT_INCLUDE_DIR ${KISSFFT_INCLUDEDIR})

            break()
          endif()
        endforeach()
      else()

        find_path(KISSFFT_INCLUDE_DIR NAMES kissfft/kiss_fft.h kissfft/kiss_fftr.h
                                      HINTS ${DEPENDS_PATH}/include
                                      ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
        find_library(KISSFFT_LIBRARY_RELEASE NAMES kissfft-float kissfft-int32 kissfft-int16 kissfft-simd
                                             HINTS ${DEPENDS_PATH}/lib
                                             ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
      endif()
    else()
      # kissfft::kissfft is an alias. We need to get the actual alias target, as we cant make an
      # alias of an alias (ie our kissfft::libkissfft cant be an alias of kissfft::kissfft)
      get_target_property(_KISSFFT_ALIASTARGET kissfft::kissfft ALIASED_TARGET)

      get_target_property(_KISSFFT_CONFIGURATIONS ${_KISSFFT_ALIASTARGET} IMPORTED_CONFIGURATIONS)

      foreach(_kissfft_config IN LISTS _KISSFFT_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_kissfft_config} _kissfft_config_UPPER)
        if((NOT ${_kissfft_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_kissfft_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(KISSFFT_LIBRARY_RELEASE ${_KISSFFT_ALIASTARGET} IMPORTED_LOCATION_${_kissfft_config_UPPER})
        else()
          get_target_property(KISSFFT_LIBRARY_${_kissfft_config_UPPER} ${_KISSFFT_ALIASTARGET} IMPORTED_LOCATION_${_kissfft_config_UPPER})
        endif()
      endforeach()

      # Need this, as we may only get the existing TARGET from system and not build or use pkg-config
      get_target_property(KISSFFT_INCLUDE_DIR ${_KISSFFT_ALIASTARGET} INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(KISSFFT)
  # Force unset _LIBRARIES as we do not want them due to our old macro usage that
  # relied on variables to populate data instead of TARGETS
  unset(KISSFFT_LIBRARIES)

  # Check if all REQUIRED_VARS are satisfied and set KISSFFT_FOUND
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(KissFFT REQUIRED_VARS KISSFFT_LIBRARY KISSFFT_INCLUDE_DIR
                                            VERSION_VAR KISSFFT_VERSION)

  if(KISSFFT_FOUND)
    if(TARGET kissfft::kissfft AND NOT TARGET build-kissfft)
      # kissfft::kissfft is an alias. We need to get the actual alias target, as we cant make an
      # alias of an alias (ie our kissfft::libkissfft cant be an alias of kissfft::kissfft)
      if(NOT _KISSFFT_ALIASTARGET)
        get_target_property(_KISSFFT_ALIASTARGET kissfft::kissfft ALIASED_TARGET)
      endif()

      add_library(kissfft::libkissfft ALIAS ${_KISSFFT_ALIASTARGET})
    else()
      # Either pkg-config or internal build, we manually set
      add_library(kissfft::libkissfft UNKNOWN IMPORTED)
      set_target_properties(kissfft::libkissfft PROPERTIES
                                                INTERFACE_INCLUDE_DIRECTORIES "${KISSFFT_INCLUDE_DIR}")

      if(KISSFFT_LIBRARY_RELEASE)
        set_target_properties(kissfft::libkissfft PROPERTIES
                                                  IMPORTED_CONFIGURATIONS RELEASE
                                                  IMPORTED_LOCATION_RELEASE "${KISSFFT_LIBRARY_RELEASE}")
      endif()
      if(KISSFFT_LIBRARY_DEBUG)
        set_target_properties(kissfft::libkissfft PROPERTIES
                                                  IMPORTED_CONFIGURATIONS DEBUG
                                                  IMPORTED_LOCATION_DEBUG "${KISSFFT_LIBRARY_DEBUG}")
      endif()
    endif()

    if(TARGET build-kissfft)
      add_dependencies(kissfft::libkissfft build-kissfft)
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
      if(NOT TARGET build-kissfft)
        buildKissFFT()
        set_target_properties(build-kissfft PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends build-kissfft)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP kissfft::libkissfft)
  endif()
endif()
