#.rst:
# FindLibtorrent
# -------
# Finds the libtorrent library
#
# This will define the following variables::
#
#   LIBTORRENT_FOUND - system has libtorrent
#   LIBTORRENT_INCLUDE_DIRS - the libtorrent include directory
#   LIBTORRENT_LIBRARIES - the libtorrent libraries
#   LIBTORRENT_DEFINITIONS - the libtorrent definitions
#
# and the following imported targets::
#
#   Libtorrent::Libtorrent - The libtorrent library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET Libtorrent::Libtorrent)
  find_package(BOOST REQUIRED)
  find_package(LibDataChannel REQUIRED)
  find_package(try_signal REQUIRED)

  set(LIBTORRENT_DEPENDENCIES
    boost::boost
    LibDataChannel::LibDataChannel
    try_signal::try_signal)

  if(ENABLE_INTERNAL_LIBTORRENT)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libtorrent)

    SETUP_BUILD_VARS()

    set(LIBTORRENT_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DCMAKE_CXX_STANDARD=17
                   -Ddeprecated-functions=OFF
                   -Dwebtorrent=ON)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Use-system-libdatachannel-and-try_signal.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Disable-asserts.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()

    add_dependencies(${MODULE_LC} ${LIBTORRENT_DEPENDENCIES})
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBTORRENT_INCLUDE_DIR libtorrent/libtorrent.hpp)
    find_library(LIBTORRENT_LIBRARY NAMES torrent-rasterbar)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libtorrent
    REQUIRED_VARS
      LIBTORRENT_LIBRARY
      LIBTORRENT_INCLUDE_DIR
    VERSION_VAR
      LIBTORRENT_VERSION)

  if(LIBTORRENT_FOUND)
    set(LIBTORRENT_INCLUDE_DIRS "${LIBTORRENT_INCLUDE_DIR}"
                                ${BOOST_INCLUDE_DIRS})
    set(LIBTORRENT_LIBRARIES "${LIBTORRENT_LIBRARY}"
                             ${BOOST_LIBRARIES})
    set(LIBTORRENT_DEFINITIONS "-DTORRENT_ABI_VERSION=100"
                               "-DTORRENT_USE_LIBCRYPTO"
                               "-DTORRENT_USE_OPENSSL"
                               ${BOOST_DEFINITIONS})

    add_library(Libtorrent::Libtorrent UNKNOWN IMPORTED)

    set_target_properties(Libtorrent::Libtorrent PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBTORRENT_LIBRARY}"
      # TODO
      #INTERFACE_COMPILE_DEFINITIONS "TORRENT_USE_OPENSSL"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBTORRENT_INCLUDE_DIR}")

    add_dependencies(Libtorrent::Libtorrent ${LIBTORRENT_DEPENDENCIES})

    if(TARGET libtorrent)
      add_dependencies(Libtorrent::Libtorrent libtorrent)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Libtorrent::Libtorrent)
  else()
    if(LIBTORRENT_FIND_REQUIRED)
      message(FATAL_ERROR "libtorrent not found. Maybe use -DENABLE_INTERNAL_LIBTORRENT=ON")
    endif()
  endif()

  mark_as_advanced(LIBTORRENT_INCLUDE_DIR)
  mark_as_advanced(LIBTORRENT_LIBRARY)
  mark_as_advanced(LIBTORRENT_DEPENDENCIES)
endif()
