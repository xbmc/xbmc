#.rst:
# FindLibDataChannel
# -------
# Finds the libdatachannel library
#
# This will define the following variables::
#
#   LIBDATACHANNEL_FOUND - system has libdatachannel
#   LIBDATACHANNEL_INCLUDE_DIRS - the libdatachannel include directory
#   LIBDATACHANNEL_LIBRARIES - the libdatachannel libraries
#   LIBDATACHANNEL_DEFINITIONS - the libdatachannel definitions
#
# and the following imported targets::
#
#   LibDataChannel::LibDataChannel - The libdatachannel library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET LibDataChannel::LibDataChannel)
  find_package(Libjuice REQUIRED)
  find_package(Libsrtp REQUIRED)
  find_package(Plog REQUIRED)
  find_package(Usrsctp REQUIRED)

  set(LIBDATACHANNEL_DEPENDENCIES
    Libjuice::Libjuice
    libsrtp::libsrtp
    plog::plog
    Usrsctp::Usrsctp)

  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libdatachannel)

    SETUP_BUILD_VARS()

    set(LIBDATACHANNEL_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DNO_EXAMPLES=ON
                   -DNO_TESTS=ON
                   -DPREFER_SYSTEM_LIB=ON)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Look-for-usrsctp-in-subdirectory.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Rename-static-library-and-exclude-shared-library.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()

    add_dependencies(${MODULE_LC} ${LIBDATACHANNEL_DEPENDENCIES})
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBDATACHANNEL_INCLUDE_DIR rtc/rtc.hpp)
    find_library(LIBDATACHANNEL_LIBRARY NAMES datachannel)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibDataChannel
    REQUIRED_VARS
      LIBDATACHANNEL_LIBRARY
      LIBDATACHANNEL_INCLUDE_DIR
    VERSION_VAR
      LIBDATACHANNEL_VERSION)

  if(LIBDATACHANNEL_FOUND)
    set(LIBDATACHANNEL_INCLUDE_DIRS "${LIBDATACHANNEL_INCLUDE_DIR}")
    set(LIBDATACHANNEL_LIBRARIES "${LIBDATACHANNEL_LIBRARY}")
    set(LIBDATACHANNEL_DEFINITIONS -DRTC_STATIC)

    add_library(LibDataChannel::LibDataChannel UNKNOWN IMPORTED)

    set_target_properties(LibDataChannel::LibDataChannel PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBDATACHANNEL_LIBRARY}"
      # TODO
      #INTERFACE_COMPILE_DEFINTIONS "RTC_STATIC"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBDATACHANNEL_INCLUDE_DIR}")

    add_dependencies(LibDataChannel::LibDataChannel ${LIBDATACHANNEL_DEPENDENCIES})

    if(TARGET libdatachannel)
      add_dependencies(LibDataChannel::LibDataChannel libdatachannel)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibDataChannel::LibDataChannel)
  else()
    if(LIBDATACHANNEL_FIND_REQUIRED)
      message(FATAL_ERROR "libdatachannel not found. Maybe use -DENABLE_INTERNAL_LIBDATACHANNEL=ON")
    endif()
  endif()

  mark_as_advanced(LIBDATACHANNEL_INCLUDE_DIR)
  mark_as_advanced(LIBDATACHANNEL_LIBRARY)
  mark_as_advanced(LIBDATACHANNEL_DEPENDENCIES)
endif()
