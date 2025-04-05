#.rst:
# FindNlohmannJSON
# -----------
# Finds the NlohmannJSON library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::NlohmannJSON - The NlohmannJSON library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildnlohmannjson)
    set(nlohmann_json_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(CMAKE_ARGS -DJSON_BuildTests=OFF)
    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/nlohmann/json.hpp)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC nlohmann_json)

  SETUP_BUILD_VARS()

  if(NlohmannJSON_FIND_VERSION)
    if(NlohmannJSON_FIND_VERSION_EXACT)
      set(NlohmannJSON_FIND_SPEC "=${NlohmannJSON_FIND_VERSION_COMPLETE}")
      set(NlohmannJSON_CONFIG_SPEC "${NlohmannJSON_FIND_VERSION_COMPLETE}" EXACT)
    else()
      set(NlohmannJSON_FIND_SPEC ">=${NlohmannJSON_FIND_VERSION_COMPLETE}")
      set(NlohmannJSON_CONFIG_SPEC "${NlohmannJSON_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(nlohmann_json CONFIG ${NlohmannJSON_CONFIG_SPEC}
                             HINTS ${DEPENDS_PATH}/share/cmake
                             ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing NLOHMANNJSON. If version >= NLOHMANNJSON-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal tinyxml2, build anyway
  if((nlohmann_json_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_NLOHMANNJSON) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_NLOHMANNJSON))
    # Build internal nlohmann_json
    buildnlohmannjson()
  else()
    get_target_property(NLOHMANN_JSON_INCLUDE_DIR nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NlohmannJSON
                                    REQUIRED_VARS NLOHMANN_JSON_INCLUDE_DIR
                                    VERSION_VAR nlohmann_json_VERSION)

  if(NLOHMANNJSON_FOUND)
    if(TARGET nlohmann_json)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}")
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS nlohmann_json::nlohmann_json)
    endif()
    if(TARGET nlohmann_json)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} nlohmann_json)
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
      if(NOT TARGET nlohmann_json)
        buildnlohmannjson()
        set_target_properties(nlohmann_json PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends nlohmann_json)
    endif()
  else()
    if(NlohmannJSON_FIND_REQUIRED)
      message(FATAL_ERROR "NlohmannJSON library not found. You may want to try -DENABLE_INTERNAL_NLOHMANNJSON=ON")
    endif()
  endif()
endif()
