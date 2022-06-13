# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following variables::
#
# FMT_FOUND - system has Fmt
# FMT_INCLUDE_DIRS - the Fmt include directory
# FMT_LIBRARIES - the Fmt libraries
#
# and the following imported targets::
#
#   fmt::fmt   - The Fmt library

# If target exists, no need to rerun find
# Allows a module that may be a dependency for multiple libraries to just be executed
# once to populate all required variables/targets
if(NOT TARGET fmt::fmt)
  if(ENABLE_INTERNAL_FMT)

    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC fmt)

    SETUP_BUILD_VARS()

    # Check for existing FMT. If version >= FMT-VERSION file version, dont build
    find_package(FMT CONFIG QUIET)

    if(FMT_VERSION VERSION_LESS ${${MODULE}_VER})

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
    else()
      # Populate paths for find_package_handle_standard_args
      find_path(FMT_INCLUDE_DIR NAMES fmt/format.h)
      find_library(FMT_LIBRARY_RELEASE NAMES fmt)
      find_library(FMT_LIBRARY_DEBUG NAMES fmtd)
    endif()
  else()
    find_package(FMT 6.1.2 CONFIG REQUIRED QUIET)

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_FMT libfmt QUIET)
      if(PC_FMT_VERSION AND NOT FMT_VERSION)
        set(FMT_VERSION ${PC_FMT_VERSION})
      endif()
    endif()

    find_path(FMT_INCLUDE_DIR NAMES fmt/format.h
                              PATHS ${PC_FMT_INCLUDEDIR})

    find_library(FMT_LIBRARY_RELEASE NAMES fmt
                                    PATHS ${PC_FMT_LIBDIR})
    find_library(FMT_LIBRARY_DEBUG NAMES fmtd
                                   PATHS ${PC_FMT_LIBDIR})

  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(FMT)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
                                    VERSION_VAR FMT_VERSION)

  if(FMT_FOUND)
    set(FMT_LIBRARIES ${FMT_LIBRARY})
    set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

    if(NOT TARGET fmt::fmt)
      add_library(fmt::fmt UNKNOWN IMPORTED)
      if(FMT_LIBRARY_RELEASE)
        set_target_properties(fmt::fmt PROPERTIES
                                       IMPORTED_CONFIGURATIONS RELEASE
                                       IMPORTED_LOCATION "${FMT_LIBRARY_RELEASE}")
      endif()
      if(FMT_LIBRARY_DEBUG)
        set_target_properties(fmt::fmt PROPERTIES
                                       IMPORTED_CONFIGURATIONS DEBUG
                                       IMPORTED_LOCATION "${FMT_LIBRARY_DEBUG}")
      endif()
      set_target_properties(fmt::fmt PROPERTIES
                                     INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
    endif()
    if(TARGET fmt)
      add_dependencies(fmt::fmt fmt)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP fmt::fmt)
  endif()

  mark_as_advanced(FMT_INCLUDE_DIR FMT_LIBRARY)
endif()
