#.rst:
# FindRapidJSON
# -----------
# Finds the RapidJSON library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::RapidJSON - The RapidJSON library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildrapidjson)
    set(BUILD_NAME rapidjson_build)

    set(RapidJSON_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/001-remove_custom_cxx_flags.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/002-cmake-standardise_config_installpath.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/003-cmake-removedocs-examples.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/004-win-arm64.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DRAPIDJSON_BUILD_DOC=OFF
                   -DRAPIDJSON_BUILD_EXAMPLES=OFF
                   -DRAPIDJSON_BUILD_TESTS=OFF
                   -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF)

    # rapidjson hasnt tagged a release since 2016. Force this for cmake 4.0
    list(APPEND CMAKE_ARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.10)

    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/rapidjson/rapidjson.h)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC rapidjson)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  find_package(RapidJSON ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                         HINTS ${DEPENDS_PATH}/lib/cmake
                         ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available (eg Debian libpcre2-dev package)
  # fallback to pkgconfig for non windows platforms
  if(NOT RapidJSON_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(RapidJSON RapidJSON${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing RAPIDJSON. If version >= RAPIDJSON-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal tinyxml2, build anyway
  if((RapidJSON_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_RapidJSON) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_RapidJSON))
    # Build internal rapidjson
    buildrapidjson()
  else()
    if(TARGET PkgConfig::RapidJSON)
      get_target_property(RAPIDJSON_INCLUDE_DIR PkgConfig::RapidJSON INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET rapidjson)
      # RapidJSON cmake config newer than 1.1.0 tag creates a target
      get_target_property(RAPIDJSON_INCLUDE_DIR rapidjson INTERFACE_INCLUDE_DIRECTORIES)
    elseif(RAPIDJSON_INCLUDE_DIRS)
      # RapidJSON cmake config <= 1.1.0 tag does not create a target, only the single variable
      set(RAPIDJSON_INCLUDE_DIR ${RAPIDJSON_INCLUDE_DIRS})
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RapidJSON
                                    REQUIRED_VARS RAPIDJSON_INCLUDE_DIR
                                    VERSION_VAR RapidJSON_VERSION)

  if(RapidJSON_FOUND)
    if(TARGET PkgConfig::RapidJSON AND NOT TARGET rapidjson_build)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::RapidJSON)
    elseif(TARGET rapidson AND NOT TARGET rapidjson_build)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS rapidson)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_INCLUDE_DIR}")
    endif()

    if(TARGET rapidjson_build)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} rapidjson_build)
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
      if(NOT TARGET rapidjson_build)
        buildrapidjson()
        set_target_properties(rapidjson_build PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends rapidjson_build)
    endif()
  else()
    if(RapidJSON_FIND_REQUIRED)
      message(FATAL_ERROR "RapidJSON library not found. You may want to try -DENABLE_INTERNAL_RapidJSON=ON")
    endif()
  endif()
endif()
