# FindCrossGUID
# -------
# Finds the CrossGUID library
#
# This will define the following target:
#
#   CrossGUID::CrossGUID   - The CrossGUID library

macro(buildCrossGUID)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  set(CROSSGUID_VERSION ${${MODULE}_VER})
  set(CROSSGUID_DEBUG_POSTFIX "-dgb")

  set(_crossguid_definitions HAVE_NEW_CROSSGUID)

  if(ANDROID)
    list(APPEND _crossguid_definitions GUID_ANDROID)
  endif()

  set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/001-fix-unused-function.patch"
              "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/002-disable-Wall-error.patch"
              "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/003-add-cstdint-include.patch")

  generate_patchcommand("${patches}")

  set(CMAKE_ARGS -DCROSSGUID_TESTS=OFF
                 -DDISABLE_WALL=ON)

  BUILD_DEP_TARGET()
endmacro()

if(NOT TARGET CrossGUID::CrossGUID)
  if(ENABLE_INTERNAL_CROSSGUID)
    buildCrossGUID()
  else()
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_CROSSGUID crossguid QUIET)
      set(CROSSGUID_VERSION ${PC_CROSSGUID_VERSION})
    endif()

    find_path(CROSSGUID_INCLUDE_DIR NAMES crossguid/guid.hpp guid.h
                                    HINTS ${DEPENDS_PATH}/include ${PC_CROSSGUID_INCLUDEDIR})
    find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid
                                           HINTS ${DEPENDS_PATH}/lib ${PC_CROSSGUID_LIBDIR})
    find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd crossguid-dgb
                                         HINTS ${DEPENDS_PATH}/lib ${PC_CROSSGUID_LIBDIR})

    # NEW_CROSSGUID >= 0.2.0 release
    if(EXISTS "${CROSSGUID_INCLUDE_DIR}/crossguid/guid.hpp")
      list(APPEND _crossguid_definitions HAVE_NEW_CROSSGUID)
    endif()
  endif()

  # Select relevant lib build type (ie CROSSGUID_LIBRARY_RELEASE or CROSSGUID_LIBRARY_DEBUG)
  include(SelectLibraryConfigurations)
  select_library_configurations(CROSSGUID)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                    VERSION_VAR CROSSGUID_VERSION)

  add_library(CrossGUID::CrossGUID UNKNOWN IMPORTED)
  if(CROSSGUID_LIBRARY_RELEASE)
    set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                               IMPORTED_CONFIGURATIONS RELEASE
                                               IMPORTED_LOCATION_RELEASE "${CROSSGUID_LIBRARY_RELEASE}")
  endif()
  if(CROSSGUID_LIBRARY_DEBUG)
    set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                               IMPORTED_CONFIGURATIONS DEBUG
                                               IMPORTED_LOCATION_DEBUG "${CROSSGUID_LIBRARY_DEBUG}")
  endif()
  set_target_properties(CrossGUID::CrossGUID PROPERTIES
                                             INTERFACE_INCLUDE_DIRECTORIES "${CROSSGUID_INCLUDE_DIRS}"
                                             INTERFACE_COMPILE_DEFINITIONS "${_crossguid_definitions}")

  if(UNIX AND NOT (APPLE OR ANDROID))
    # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    set(FPHSA_NAME_MISMATCHED 1)
    find_package(UUID REQUIRED)
    unset(FPHSA_NAME_MISMATCHED)

    if(TARGET UUID::UUID)
      add_dependencies(CrossGUID::CrossGUID UUID::UUID)
      target_link_libraries(CrossGUID::CrossGUID INTERFACE UUID::UUID)
    endif()
  endif()

  if(TARGET crossguid)
    add_dependencies(CrossGUID::CrossGUID crossguid)
  else()
    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build 
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      buildCrossGUID()
    endif()
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP CrossGUID::CrossGUID)
endif()
mark_as_advanced(CROSSGUID_INCLUDE_DIR CROSSGUID_LIBRARY)
