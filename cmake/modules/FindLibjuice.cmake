#.rst:
# FindLibjuice
# -------
# Finds the libjuice library
#
# This will define the following variables::
#
#   LIBJUICE_FOUND - system has libjuice
#   LIBJUICE_INCLUDE_DIRS - the libjuice include directory
#   LIBJUICE_LIBRARIES - the libjuice libraries
#
# and the following imported targets::
#
#   Libjuice::Libjuice - The libjuice library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET Libjuice::Libjuice)
  if(ENABLE_INTERNAL_LIBJUICE)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libjuice)

    SETUP_BUILD_VARS()

    set(LIBJUICE_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DNO_TESTS=ON)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Don-t-exclude-static-library.patch")

    generate_patchcommand("${patches}")

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBJUICE_INCLUDE_DIR juice/juice.h)
    find_library(LIBJUICE_LIBRARY NAMES juice)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libjuice
    REQUIRED_VARS
      LIBJUICE_LIBRARY
      LIBJUICE_INCLUDE_DIR
    VERSION_VAR
      LIBJUICE_VERSION)

  if(LIBJUICE_FOUND)
    set(LIBJUICE_INCLUDE_DIRS "${LIBJUICE_INCLUDE_DIR}")
    set(LIBJUICE_LIBRARIES "${LIBJUICE_LIBRARY}")

    add_library(Libjuice::Libjuice UNKNOWN IMPORTED)

    set_target_properties(Libjuice::Libjuice PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBJUICE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBJUICE_INCLUDE_DIR}")

    if(TARGET libjuice)
      add_dependencies(Libjuice::Libjuice libjuice)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Libjuice::Libjuice)
  else()
    if(LIBJUICE_FIND_REQUIRED)
      message(FATAL_ERROR "libjuice not found. Maybe use -DENABLE_INTERNAL_LIBJUICE=ON")
    endif()
  endif()

  mark_as_advanced(LIBJUICE_INCLUDE_DIR)
  mark_as_advanced(LIBJUICE_LIBRARY)
endif()
