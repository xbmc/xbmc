#.rst:
# FindLibsrtp
# -------
# Finds the libsrtp library
#
# This will define the following variables::
#
#   LIBSRTP_FOUND - system has libsrtp
#   LIBSRTP_INCLUDE_DIRS - the libsrtp include directory
#   LIBSRTP_LIBRARIES - the libsrtp libraries
#
# and the following imported targets::
#
#   libsrtp::libsrtp - The libsrtp library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET libsrtp::libsrtp)
  if(ENABLE_INTERNAL_LIBSRTP)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libsrtp)

    SETUP_BUILD_VARS()

    set(LIBSRTP_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DBUILD_WITH_WARNINGS=OFF
                   -DENABLE_OPENSSL=ON
                   -DLIBSRTP_TEST_APPS=OFF)

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBSRTP_INCLUDE_DIR NAMES srtp2/srtp.h)
    find_library(LIBSRTP_LIBRARY NAMES srtp2)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libsrtp
    REQUIRED_VARS
      LIBSRTP_LIBRARY
      LIBSRTP_INCLUDE_DIR
    VERSION_VAR
      LIBSRTP_VERSION)

  if(LIBSRTP_FOUND)
    set(LIBSRTP_INCLUDE_DIRS "${LIBSRTP_INCLUDE_DIR}")
    set(LIBSRTP_LIBRARIES "${LIBSRTP_LIBRARY}")

    add_library(libsrtp::libsrtp UNKNOWN IMPORTED)

    set_target_properties(libsrtp::libsrtp PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBSRTP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBSRTP_INCLUDE_DIR}")

    if(TARGET libsrtp)
      add_dependencies(libsrtp::libsrtp libsrtp)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libsrtp::libsrtp)
  else()
    if(LIBSRTP_FIND_REQUIRED)
      message(FATAL_ERROR "libsrtp not found. Maybe use -DENABLE_INTERNAL_LIBSRTP=ON")
    endif()
  endif()

  mark_as_advanced(LIBSRTP_INCLUDE_DIR)
  mark_as_advanced(LIBSRTP_LIBRARY)
endif()
