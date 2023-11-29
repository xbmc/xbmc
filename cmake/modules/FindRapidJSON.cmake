#.rst:
# FindRapidJSON
# -----------
# Finds the RapidJSON library
#
# This will define the following target:
#
#   RapidJSON::RapidJSON - The RapidJSON library
#

if(NOT TARGET RapidJSON::RapidJSON)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildrapidjson)
    set(RapidJSON_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/001-remove_custom_cxx_flags.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/002-cmake-standardise_config_installpath.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/003-cmake-removedocs-examples.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/004-win-arm64.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DRAPIDJSON_BUILD_DOC=OFF
                   -DRAPIDJSON_BUILD_EXAMPLES=OFF
                   -DRAPIDJSON_BUILD_TESTS=OFF
                   -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF)

    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/rapidjson/rapidjson.h)

    BUILD_DEP_TARGET()

    set(RAPIDJSON_INCLUDE_DIRS ${${MODULE}_INCLUDE_DIR})
  endmacro()

  set(MODULE_LC rapidjson)

  SETUP_BUILD_VARS()

  if(RapidJSON_FIND_VERSION)
    if(RapidJSON_FIND_VERSION_EXACT)
      set(RapidJSON_FIND_SPEC "=${RapidJSON_FIND_VERSION_COMPLETE}")
      set(RapidJSON_CONFIG_SPEC "${RapidJSON_FIND_VERSION_COMPLETE}" EXACT)
    else()
      set(RapidJSON_FIND_SPEC ">=${RapidJSON_FIND_VERSION_COMPLETE}")
      set(RapidJSON_CONFIG_SPEC "${RapidJSON_FIND_VERSION_COMPLETE}")
    endif()
  endif()

  find_package(RapidJSON CONFIG ${RapidJSON_CONFIG_SPEC}
                                HINTS ${DEPENDS_PATH}/lib/cmake
                                ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing RAPIDJSON. If version >= RAPIDJSON-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal tinyxml2, build anyway
  if((RapidJSON_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_RapidJSON) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_RapidJSON))
    # Build internal rapidjson
    buildrapidjson()
  else()
    # If RAPIDJSON_INCLUDE_DIRS exists, then the find_package command found a config
    # and suitable version. If its not, we fall back to a pkgconfig/manual search
    if(NOT DEFINED RAPIDJSON_INCLUDE_DIRS)
      find_package(PkgConfig)
      # Fallback to pkg-config and individual lib/include file search
      # Do not use pkgconfig on windows
      if(PKG_CONFIG_FOUND AND NOT WIN32)
        pkg_check_modules(PC_RapidJSON RapidJSON${RapidJSON_FIND_SPEC} QUIET)
        set(RapidJSON_VERSION ${PC_RapidJSON_VERSION})
      endif()

      find_path(RAPIDJSON_INCLUDE_DIRS NAMES rapidjson/rapidjson.h
                                       HINTS ${DEPENDS_PATH}/include ${PC_RapidJSON_INCLUDEDIR}
                                       ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG}
                                       NO_CACHE)
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RapidJSON
                                    REQUIRED_VARS RAPIDJSON_INCLUDE_DIRS RapidJSON_VERSION
                                    VERSION_VAR RapidJSON_VERSION)

  if(RAPIDJSON_FOUND)
    add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON::RapidJSON PROPERTIES
                                               INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_INCLUDE_DIRS}")
    if(TARGET rapidjson)
      add_dependencies(RapidJSON::RapidJSON rapidjson)
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
      if(NOT TARGET rapidjson)
        buildrapidjson()
        set_target_properties(rapidjson PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends rapidjson)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP RapidJSON::RapidJSON)
  endif()
endif()
