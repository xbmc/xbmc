# FindCrossGUID
# -------
# Finds the CrossGUID library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::CrossGUID   - The CrossGUID library

macro(buildCrossGUID)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  set(CROSSGUID_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX "-dgb")

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

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(ENABLE_INTERNAL_CROSSGUID)
    buildCrossGUID()
  else()
    find_package(PkgConfig ${SEARCH_QUIET})
    # Do not use pkgconfig on windows
    if(PKG_CONFIG_FOUND AND NOT WIN32)
      pkg_check_modules(PC_CROSSGUID crossguid ${SEARCH_QUIET})
      set(CROSSGUID_VERSION ${PC_CROSSGUID_VERSION})
    endif()

    find_path(CROSSGUID_INCLUDE_DIR NAMES crossguid/guid.hpp guid.h
                                    HINTS ${DEPENDS_PATH}/include ${PC_CROSSGUID_INCLUDEDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid
                                           HINTS ${DEPENDS_PATH}/lib ${PC_CROSSGUID_LIBDIR}
                                           ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd crossguid-dgb
                                         HINTS ${DEPENDS_PATH}/lib ${PC_CROSSGUID_LIBDIR}
                                         ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    # NEW_CROSSGUID >= 0.2.0 release
    if(EXISTS "${CROSSGUID_INCLUDE_DIR}/crossguid/guid.hpp")
      list(APPEND _crossguid_definitions HAVE_NEW_CROSSGUID)
    endif()
  endif()

  # Select relevant lib build type (ie CROSSGUID_LIBRARY_RELEASE or CROSSGUID_LIBRARY_DEBUG)
  include(SelectLibraryConfigurations)
  select_library_configurations(CROSSGUID)
  unset(CROSSGUID_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                    VERSION_VAR CROSSGUID_VERSION)

  if(CROSSGUID_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(CROSSGUID_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${CROSSGUID_LIBRARY_RELEASE}")
    endif()
    if(CROSSGUID_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${CROSSGUID_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${CROSSGUID_INCLUDE_DIRS}"
                                                                     INTERFACE_COMPILE_DEFINITIONS "${_crossguid_definitions}")

    if(UNIX AND NOT (APPLE OR ANDROID))
      # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
      set(FPHSA_NAME_MISMATCHED 1)
      find_package(UUID REQUIRED ${SEARCH_QUIET})
      unset(FPHSA_NAME_MISMATCHED)

      if(TARGET UUID::UUID)
        add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UUID::UUID)
        target_link_libraries(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE UUID::UUID)
      endif()
    endif()

    if(TARGET crossguid)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} crossguid)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET crossguid)
        buildCrossGUID()
        set_target_properties(crossguid PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends crossguid)
    endif()
  else()
    if(CrossGUID_FIND_REQUIRED)
      message(FATAL_ERROR "CrossGUID libraries were not found. You may want to use -DENABLE_INTERNAL_CROSSGUID=ON")
    endif()
  endif()
endif()
