# FindCrossGUID
# -------
# Finds the CrossGUID library
#
# This will define the following variables::
#
# CROSSGUID_FOUND_FOUND - system has CrossGUID
# CROSSGUID_INCLUDE_DIRS - the CrossGUID include directory
# CROSSGUID_LIBRARIES - the CrossGUID libraries
# CROSSGUID_DEFINITIONS - cmake definitions required
#
# and the following imported targets::
#
#   crossguid   - The CrossGUID library

if(ENABLE_INTERNAL_CROSSGUID)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC crossguid)

  # Temp: We force CMAKE_BUILD_TYPE to release, and makefile builds respect this
  # Multi config generators (eg VS, Xcode) dont, so handle debug postfix build/link for them only
  if(NOT CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    set(CROSSGUID_DEBUG_POSTFIX "-dgb")
  endif()

  SETUP_BUILD_VARS()

  set(CROSSGUID_VERSION ${${MODULE}_VER})
  set(CROSSGUID_DEFINITIONS -DHAVE_NEW_CROSSGUID)

  if(ANDROID)
    list(APPEND CROSSGUID_DEFINITIONS -DGUID_ANDROID)
  endif()

  # Use custom findpatch to handle windows patch binary if not available
  include(cmake/modules/FindPatch.cmake)

  set(PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/001-fix-unused-function.patch
            COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/002-disable-Wall-error.patch)

  # Force release build type. crossguid forces a debug postfix -dgb. may want to patch this
  # if we enable adaptive build type for the library.
  set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                 -DCROSSGUID_TESTS=OFF
                 -DDISABLE_WALL=ON
                 -DCMAKE_BUILD_TYPE=Release)

  BUILD_DEP_TARGET()

else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CROSSGUID crossguid REQUIRED QUIET)
    set(CROSSGUID_VERSION ${PC_CROSSGUID_VERSION})
  endif()

  find_path(CROSSGUID_INCLUDE_DIR NAMES crossguid/guid.hpp guid.h
                                  PATHS ${PC_CROSSGUID_INCLUDEDIR})

  find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid
                                         PATHS ${PC_CROSSGUID_LIBDIR})
  find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd crossguid-dgb
                                       PATHS ${PC_CROSSGUID_LIBDIR})

endif()

# Select relevant lib build type (ie CROSSGUID_LIBRARY_RELEASE or CROSSGUID_LIBRARY_DEBUG)
include(SelectLibraryConfigurations)
select_library_configurations(CROSSGUID)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CrossGUID
                                  REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                  VERSION_VAR CROSSGUID_VERSION)

if(CROSSGUID_FOUND)
  set(CROSSGUID_LIBRARIES ${CROSSGUID_LIBRARY})
  set(CROSSGUID_INCLUDE_DIRS ${CROSSGUID_INCLUDE_DIR})

  # NEW_CROSSGUID >= 0.2.0 release
  if(EXISTS "${CROSSGUID_INCLUDE_DIR}/crossguid/guid.hpp")
    list(APPEND CROSSGUID_DEFINITIONS -DHAVE_NEW_CROSSGUID)
  endif()

  if(NOT TARGET crossguid)
    add_library(crossguid UNKNOWN IMPORTED)
    set_target_properties(crossguid PROPERTIES
                                    IMPORTED_LOCATION "${CROSSGUID_LIBRARY}"
                                    INTERFACE_INCLUDE_DIRECTORIES "${CROSSGUID_INCLUDE_DIR}")
  endif()

  if(UNIX AND NOT (APPLE OR ANDROID))
    # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    set(FPHSA_NAME_MISMATCHED 1)
    find_package(UUID REQUIRED)
    unset(FPHSA_NAME_MISMATCHED)
    list(APPEND CROSSGUID_INCLUDE_DIRS ${UUID_INCLUDE_DIRS})
    list(APPEND CROSSGUID_LIBRARIES ${UUID_LIBRARIES})
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP crossguid)
endif()
mark_as_advanced(CROSSGUID_INCLUDE_DIR CROSSGUID_LIBRARY)
