#.rst:
# Findtry_signal
# -------
# Finds the try_signal library
#
# This will define the following variables::
#
#   TRY_SIGNAL_FOUND - system has usrsctp
#   TRY_SIGNAL_INCLUDE_DIRS - the usrsctp include directory
#   TRY_SIGNAL_LIBRARIES - the usrsctp libraries
#
# and the following imported targets::
#
#   try_signal::try_signal - The try_signal library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET try_signal::try_signal)
  if(ENABLE_INTERNAL_TRY_SIGNAL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC try_signal)

    SETUP_BUILD_VARS()

    set(TRY_SIGNAL_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-CMake-Add-install-step.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(TRY_SIGNAL_INCLUDE_DIR NAMES try_signal.hpp)
    find_library(TRY_SIGNAL_LIBRARY NAMES try_signal)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(try_signal
    REQUIRED_VARS
      TRY_SIGNAL_LIBRARY
      TRY_SIGNAL_INCLUDE_DIR
    VERSION_VAR
      TRY_SIGNAL_VERSION)

  if(TRY_SIGNAL_FOUND)
    set(TRY_SIGNAL_INCLUDE_DIRS "${TRY_SIGNAL_INCLUDE_DIR}")
    set(TRY_SIGNAL_LIBRARIES "${TRY_SIGNAL_LIBRARY}")

    add_library(try_signal::try_signal UNKNOWN IMPORTED)

    set_target_properties(try_signal::try_signal PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${TRY_SIGNAL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TRY_SIGNAL_INCLUDE_DIR}")

    if(TARGET try_signal)
      add_dependencies(try_signal::try_signal try_signal)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP try_signal::try_signal)
  else()
    if(TRY_SIGNAL_FIND_REQUIRED)
      message(FATAL_ERROR "try_signal not found. Maybe use -DENABLE_INTERNAL_TRY_SIGNAL=ON")
    endif()
  endif()

  mark_as_advanced(TRY_SIGNAL_INCLUDE_DIR)
  mark_as_advanced(TRY_SIGNAL_LIBRARY)
endif()
