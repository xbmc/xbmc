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
#   CrossGUID::CrossGUID   - The CrossGUID library

if(ENABLE_INTERNAL_CROSSGUID)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  set(CROSSGUID_VERSION ${${MODULE}_VER})
  set(CROSSGUID_DEFINITIONS -DHAVE_NEW_CROSSGUID)
  set(CROSSGUID_DEBUG_POSTFIX "-dgb")

  if(ANDROID)
    list(APPEND CROSSGUID_DEFINITIONS -DGUID_ANDROID)
  endif()

  set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/001-fix-unused-function.patch"
              "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/002-disable-Wall-error.patch")

  generate_patchcommand("${patches}")

  set(CMAKE_ARGS -DCROSSGUID_TESTS=OFF
                 -DDISABLE_WALL=ON)

  BUILD_DEP_TARGET()

else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_CROSSGUID crossguid QUIET)
    set(CROSSGUID_VERSION ${PC_CROSSGUID_VERSION})
  endif()

  if(CROSSGUID_FOUND)
    find_path(CROSSGUID_INCLUDE_DIR NAMES crossguid/guid.hpp guid.h
    PATHS ${PC_CROSSGUID_INCLUDEDIR})

    find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid
            PATHS ${PC_CROSSGUID_LIBDIR})
    find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd crossguid-dgb
          PATHS ${PC_CROSSGUID_LIBDIR})
  else()
    find_path(CROSSGUID_INCLUDE_DIR NAMES crossguid/guid.hpp guid.h)
    find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid)
    find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd)
  endif()
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

  if(UNIX AND NOT (APPLE OR ANDROID))
    # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    set(FPHSA_NAME_MISMATCHED 1)
    find_package(UUID REQUIRED)
    unset(FPHSA_NAME_MISMATCHED)
    list(APPEND CROSSGUID_INCLUDE_DIRS ${UUID_INCLUDE_DIRS})
    list(APPEND CROSSGUID_LIBRARIES ${UUID_LIBRARIES})
  endif()

  if(NOT TARGET CrossGUID::CrossGUID)
    add_library(CrossGUID::CrossGUID UNKNOWN IMPORTED)
    if(CROSSGUID_LIBRARY_RELEASE)
      set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                                 IMPORTED_CONFIGURATIONS RELEASE
                                                 IMPORTED_LOCATION "${CROSSGUID_LIBRARY_RELEASE}")
    endif()
    if(CROSSGUID_LIBRARY_DEBUG)
      set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                                 IMPORTED_CONFIGURATIONS DEBUG
                                                 IMPORTED_LOCATION "${CROSSGUID_LIBRARY_DEBUG}")
    endif()
    set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                               INTERFACE_INCLUDE_DIRECTORIES "${CROSSGUID_INCLUDE_DIRS}")
  endif()
  if(TARGET crossguid)
    add_dependencies(CrossGUID::CrossGUID crossguid)
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CrossGUID::CrossGUID)

endif()
mark_as_advanced(CROSSGUID_INCLUDE_DIR CROSSGUID_LIBRARY)
