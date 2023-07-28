#.rst:
# FindPlog
# -------
# Finds the plog library
#
# This will define the following variables::
#
#   PLOG_FOUND - system has plog
#   PLOG_INCLUDE_DIRS - the plog include directory
#
# and the following imported targets::
#
#   plog::plog - The plog library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET plog::plog)
  if(ENABLE_INTERNAL_PLOG)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC plog)

    SETUP_BUILD_VARS()

    set(PLOG_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DPLOG_BUILD_SAMPLES=OFF
                   -DPLOG_INSTALL=ON)

    # Ninja always needs a byproduct
    set(BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/plog/Log.h)

    BUILD_DEP_TARGET()

    set(PLOG_INCLUDE_DIR ${${MODULE}_INCLUDE_DIR})
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(PLOG_INCLUDE_DIR NAMES plog/Log.h)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Plog
    REQUIRED_VARS
      PLOG_INCLUDE_DIR
    VERSION_VAR
      PLOG_VERSION)

  if(PLOG_FOUND)
    set(PLOG_INCLUDE_DIRS "${PLOG_INCLUDE_DIR}")

    add_library(plog::plog INTERFACE IMPORTED)

    set_target_properties(plog::plog PROPERTIES
      FOLDER "External Projects"
      INTERFACE_INCLUDE_DIRECTORIES "${PLOG_INCLUDE_DIR}")

    if(TARGET plog)
      add_dependencies(plog::plog plog)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP plog::plog)
  else()
    if(PLOG_FIND_REQUIRED)
      message(FATAL_ERROR "plog not found. Maybe use -DENABLE_INTERNAL_PLOG=ON")
    endif()
  endif()

  mark_as_advanced(PLOG_INCLUDE_DIR)
endif()
