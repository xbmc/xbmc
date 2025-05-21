# FindEffects11
# -------
# Finds the Effects11 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Effects11   - The Effects11 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroEffects11)

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

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    # Make INCLUDE_DIR match cmake config output
    string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR "/Effects11")
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC effects11)

  SETUP_BUILD_VARS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    get_target_property(_EFFECTS_CONFIGURATIONS Microsoft::Effects11 IMPORTED_CONFIGURATIONS)
    if(_EFFECTS_CONFIGURATIONS)
      foreach(_effects_config IN LISTS _EFFECTS_CONFIGURATIONS)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_effects_config} _effects_config_UPPER)
        if((NOT ${_effects_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_effects_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE Microsoft::Effects11 IMPORTED_LOCATION_${_effects_config_UPPER})
        else()
          get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_effects_config_UPPER} Microsoft::Effects11 IMPORTED_LOCATION_${_effects_config_UPPER})
        endif()
      endforeach()
    else()
      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE Microsoft::Effects11 IMPORTED_LOCATION)
    endif()

    get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR Microsoft::Effects11 INTERFACE_INCLUDE_DIRECTORIES)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Effects11
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Effects11_FOUND)
    
    if(TARGET Microsoft::Effects11 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Microsoft::Effects11)
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Effects11_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Effects11 library.")
    endif()
  endif()
endif()
