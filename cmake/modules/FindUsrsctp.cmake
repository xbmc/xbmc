#.rst:
# FindUsrsctp
# -------
# Finds the usrsctp library
#
# This will define the following variables::
#
#   USRSCTP_FOUND - system has usrsctp
#   USRSCTP_INCLUDE_DIRS - the usrsctp include directory
#   USRSCTP_LIBRARIES - the usrsctp libraries
#
# and the following imported targets::
#
#   Usrsctp::Usrsctp - The usrsctp library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET Usrsctp::Usrsctp)
  if(ENABLE_INTERNAL_USRSCTP)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC usrsctp)

    SETUP_BUILD_VARS()

    set(USRSCTP_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -Dsctp_build_programs=OFF
                   -Dsctp_werror=OFF)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Add-CMake-export-targets.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Also-install-library-to-subdirectory.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0003-fix-build-error-on-windows.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0004-Fix-missing-getifaddrs-and-freeifaddrs-on-Android.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(USRSCTP_INCLUDE_DIR NAMES usrsctp.h)
    find_library(USRSCTP_LIBRARY NAMES usrsctp)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Usrsctp
    REQUIRED_VARS
      USRSCTP_LIBRARY
      USRSCTP_INCLUDE_DIR
    VERSION_VAR
      USRSCTP_VERSION)

  if(USRSCTP_FOUND)
    set(USRSCTP_INCLUDE_DIRS "${USRSCTP_INCLUDE_DIR}")
    set(USRSCTP_LIBRARIES "${USRSCTP_LIBRARY}")

    add_library(Usrsctp::Usrsctp UNKNOWN IMPORTED)

    set_target_properties(Usrsctp::Usrsctp PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${USRSCTP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${USRSCTP_INCLUDE_DIR}")

    if(TARGET usrsctp)
      add_dependencies(Usrsctp::Usrsctp usrsctp)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Usrsctp::Usrsctp)
  else()
    if(USRSCTP_FIND_REQUIRED)
      message(FATAL_ERROR "usrsctp not found. Maybe use -DENABLE_INTERNAL_USRSCTP=ON")
    endif()
  endif()

  mark_as_advanced(USRSCTP_INCLUDE_DIR)
  mark_as_advanced(USRSCTP_LIBRARY)
endif()
