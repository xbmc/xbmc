# FindSpdlog
# -------
# Finds the Spdlog library
#
# This will define the following variables:
#
# SPDLOG_FOUND - system has Spdlog
# SPDLOG_INCLUDE_DIRS - the Spdlog include directory
# SPDLOG_LIBRARIES - the Spdlog libraries
# SPDLOG_DEFINITIONS - the Spdlog compile definitions
#
# and the following imported targets:
#
#   Spdlog::Spdlog   - The Spdlog library

if(ENABLE_INTERNAL_SPDLOG)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC spdlog)

  # spdlog debug uses postfix d for all platforms
  set(SPDLOG_DEBUG_POSTFIX d)

  SETUP_BUILD_VARS()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  if(WIN32 OR WINDOWS_STORE)
    # find the path to the patch executable
    find_package(Patch MODULE REQUIRED)

    set(patch ${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-windows-pdb-symbol-gen.patch)
    PATCH_LF_CHECK(${patch})

    set(PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${patch})
  endif()

  set(SPDLOG_VERSION ${${MODULE}_VER})

  set(CMAKE_ARGS -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                 -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                 -DSPDLOG_BUILD_EXAMPLE=OFF
                 -DSPDLOG_BUILD_TESTS=OFF
                 -DSPDLOG_BUILD_BENCH=OFF
                 -DSPDLOG_FMT_EXTERNAL=ON
                 -DCMAKE_PREFIX_PATH=${DEPENDS_PATH}
                 "${EXTRA_ARGS}")

  BUILD_DEP_TARGET()

  if(ENABLE_INTERNAL_FMT)
    add_dependencies(${MODULE_LC} fmt)
  else()
    # spdlog 1.9.2 fails to build with fmt < 8.0.0
    find_package(fmt 8.0.0 CONFIG REQUIRED QUIET)
  endif()
else()
  find_package(spdlog 1.5.0 CONFIG REQUIRED QUIET)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SPDLOG spdlog QUIET)
    set(SPDLOG_VERSION ${PC_SPDLOG_VERSION})
  endif()

  find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h
                               PATHS ${PC_SPDLOG_INCLUDEDIR})

  find_library(SPDLOG_LIBRARY_RELEASE NAMES spdlog
                                      PATHS ${PC_SPDLOG_LIBDIR})
  find_library(SPDLOG_LIBRARY_DEBUG NAMES spdlogd
                                    PATHS ${PC_SPDLOG_LIBDIR})
endif()

include(SelectLibraryConfigurations)
select_library_configurations(SPDLOG)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spdlog
                                  REQUIRED_VARS SPDLOG_LIBRARY SPDLOG_INCLUDE_DIR
                                  VERSION_VAR SPDLOG_VERSION)

if(SPDLOG_FOUND)
  set(SPDLOG_LIBRARIES ${SPDLOG_LIBRARY})
  set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})
  set(SPDLOG_DEFINITIONS -DSPDLOG_FMT_EXTERNAL
                         -DSPDLOG_DEBUG_ON
                         -DSPDLOG_NO_ATOMIC_LEVELS
                         -DSPDLOG_ENABLE_PATTERN_PADDING)
  if(WIN32)
    list(APPEND SPDLOG_DEFINITIONS -DSPDLOG_WCHAR_FILENAMES
                                   -DSPDLOG_WCHAR_TO_UTF8_SUPPORT)
  endif()

  if(NOT TARGET spdlog)
    add_library(spdlog UNKNOWN IMPORTED)
    set_target_properties(spdlog PROPERTIES
                                 IMPORTED_LOCATION "${SPDLOG_LIBRARY}"
                                 INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
                                 INTERFACE_COMPILE_DEFINITIONS "${SPDLOG_DEFINITIONS}")
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP spdlog)
endif()

mark_as_advanced(SPDLOG_INCLUDE_DIR SPDLOG_LIBRARY)
