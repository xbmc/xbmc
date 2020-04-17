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
#   Fmt::Fmt   - The Fmt library

if(ENABLE_INTERNAL_FMT)
  include(ExternalProject)
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/libfmt/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" FMT_VERSION "${VER}")

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(FMT_URL)
      get_filename_component(FMT_URL "${FMT_URL}" ABSOLUTE)
  else()
      set(FMT_URL http://mirrors.kodi.tv/build-deps/sources/fmt-${FMT_VERSION}.tar.gz)
  endif()
  if(VERBOSE)
      message(STATUS "FMT_URL: ${FMT_URL}")
  endif()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(FMT_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libfmt.a)
  set(FMT_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  externalproject_add(fmt
                      URL ${FMT_URL}
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/fmt
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                                 -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 -DCMAKE_INSTALL_LIBDIR=lib
                                 -DFMT_DOC=OFF
                                 -DFMT_TEST=OFF
                                 "${EXTRA_ARGS}"
                      BUILD_BYPRODUCTS ${FMT_LIBRARY})
  set_target_properties(fmt PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fmt
                                    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
                                    VERSION_VAR FMT_VERSION)

  set(FMT_LIBRARIES ${FMT_LIBRARY})
  set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

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

include(SelectLibraryConfigurations)
select_library_configurations(FMT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt
                                  REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR FMT_VERSION
                                  VERSION_VAR FMT_VERSION)

if(FMT_FOUND)
  set(FMT_LIBRARIES ${FMT_LIBRARY})
  set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

  if(NOT TARGET fmt)
    add_library(fmt UNKNOWN IMPORTED)
    set_target_properties(fmt PROPERTIES
                               IMPORTED_LOCATION "${FMT_LIBRARY}"
                               INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
  endif()
endif()

endif()
mark_as_advanced(FMT_INCLUDE_DIR FMT_LIBRARY)
