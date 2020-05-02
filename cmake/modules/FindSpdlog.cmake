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
  include(ExternalProject)
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/libspdlog/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" SPDLOG_VERSION "${VER}")

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(SPDLOG_URL)
      get_filename_component(SPDLOG_URL "${SPDLOG_URL}" ABSOLUTE)
  else()
      set(SPDLOG_URL http://mirrors.kodi.tv/build-deps/sources/spdlog-${SPDLOG_VERSION}.tar.gz)
  endif()
  if(VERBOSE)
      message(STATUS "SPDLOG_URL: ${SPDLOG_URL}")
  endif()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(SPDLOG_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libspdlog.a)
  set(SPDLOG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)

  externalproject_add(spdlog
                      URL ${SPDLOG_URL}
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PATCH_COMMAND patch -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/libspdlog/0001-fix_fmt_version.patch
                      PREFIX ${CORE_BUILD_DIR}/spdlog
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                                 -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 -DCMAKE_INSTALL_LIBDIR=lib
                                 -DSPDLOG_BUILD_EXAMPLE=OFF
                                 -DSPDLOG_BUILD_TESTS=OFF
                                 -DSPDLOG_BUILD_BENCH=OFF
                                 -DSPDLOG_FMT_EXTERNAL=ON
                                 -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 "${EXTRA_ARGS}"
                      BUILD_BYPRODUCTS ${SPDLOG_LIBRARY})
  set_target_properties(spdlog PROPERTIES FOLDER "External Projects")

  if(ENABLE_INTERNAL_FMT)
    add_dependencies(spdlog fmt)
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

  include(SelectLibraryConfigurations)
  select_library_configurations(SPDLOG)
endif()

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

  if(NOT TARGET Spdlog::Spdlog)
    add_library(Spdlog::Spdlog UNKNOWN IMPORTED)
    set_target_properties(Spdlog::Spdlog PROPERTIES
                                         IMPORTED_LOCATION "${SPDLOG_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
                                         INTERFACE_COMPILE_DEFINITIONS "${SPDLOG_DEFINITIONS}")
  endif()
endif()

mark_as_advanced(SPDLOG_INCLUDE_DIR SPDLOG_LIBRARY)
