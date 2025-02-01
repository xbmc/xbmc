# FindEffects11
# -------
# Finds the Effects11 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Effects11   - The Effects11 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildEffects11)

    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-debugpostfix.patch")
    generate_patchcommand("${patches}")

    # Effects 11 cant be built using /permissive-
    # strip and manually set the rest of the build flags
    string(REPLACE "/permissive-" "" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS ${CMAKE_CXX_FLAGS} )

    set(CMAKE_ARGS 
    "-DCMAKE_CXX_FLAGS=${EFFECTS_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>"
    "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} $<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}>")

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
    set(WIN_DISABLE_PROJECT_FLAGS ON)

    BUILD_DEP_TARGET()

    set(EFFECTS11_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    # Make INCLUDE_DIR match cmake config output
    string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR "/Effects11")
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC effects11)

  SETUP_BUILD_VARS()

  find_package(effects11 CONFIG QUIET
                                HINTS ${DEPENDS_PATH}/share
                                ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  if(effects11_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    buildEffects11()
  else()
    get_target_property(_EFFECTS_CONFIGURATIONS Microsoft::Effects11 IMPORTED_CONFIGURATIONS)
    foreach(_effects_config IN LISTS _EFFECTS_CONFIGURATIONS)
      # Some non standard config (eg None on Debian)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_effects_config} _effects_config_UPPER)
      if((NOT ${_effects_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_effects_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(EFFECTS11_LIBRARY_RELEASE Microsoft::Effects11 IMPORTED_LOCATION_${_effects_config_UPPER})
      else()
        get_target_property(EFFECTS11_LIBRARY_${_effects_config_UPPER} Microsoft::Effects11 IMPORTED_LOCATION_${_effects_config_UPPER})
      endif()
    endforeach()

    get_target_property(EFFECTS11_INCLUDE_DIR Microsoft::Effects11 INTERFACE_INCLUDE_DIRECTORIES)
    set(EFFECTS11_VERSION ${effects11_VERSION})
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(EFFECTS11)
  unset(EFFECTS11_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Effects11
                                    REQUIRED_VARS EFFECTS11_LIBRARY EFFECTS11_INCLUDE_DIR
                                    VERSION_VAR EFFECTS11_VERSION)

  if(Effects11_FOUND)
    
    if(TARGET Microsoft::Effects11 AND NOT TARGET effects11)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Microsoft::Effects11)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${EFFECTS11_INCLUDE_DIR}")

      if(EFFECTS11_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${EFFECTS11_LIBRARY_RELEASE}")
      endif()
      if(EFFECTS11_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${EFFECTS11_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()
    endif()

    if(TARGET effects11)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} effects11)
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
      if(NOT TARGET effects11)
        buildEffects11()
        set_target_properties(effects11 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends effects11)
    endif()
  else()
    if(Effects11_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Effects11 library.")
    endif()
  endif()
endif()
