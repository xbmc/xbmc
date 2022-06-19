#.rst:
# FindRapidJSON
# -----------
# Finds the RapidJSON library
#
# This will define the following variables::
#
# RapidJSON_FOUND - system has RapidJSON parser
# RapidJSON_INCLUDE_DIRS - the RapidJSON parser include directory
#
if(ENABLE_INTERNAL_RapidJSON)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC rapidjson)

  SETUP_BUILD_VARS()

  set(RapidJSON_VERSION ${${MODULE}_VER})

  set(patches "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/001-remove_custom_cxx_flags.patch"
              "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/002-cmake-removedocs-examples.patch"
              "${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/003-win-arm64.patch")

  generate_patchcommand("${patches}")

  set(CMAKE_ARGS -DRAPIDJSON_BUILD_DOC=OFF
                 -DRAPIDJSON_BUILD_EXAMPLES=OFF
                 -DRAPIDJSON_BUILD_TESTS=OFF
                 -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF)

  set(BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/rapidjson/rapidjson.h)

  BUILD_DEP_TARGET()

  set(RapidJSON_INCLUDE_DIR ${${MODULE}_INCLUDE_DIR})

  # Add dependency to libkodi to build
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP rapidjson)
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_RapidJSON RapidJSON>=1.0.2 QUIET)
  endif()

  if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(RapidJSON_VERSION 1.1.0)
  else()
    if(PC_RapidJSON_VERSION)
      set(RapidJSON_VERSION ${PC_RapidJSON_VERSION})
    else()
      find_package(RapidJSON 1.1.0 CONFIG REQUIRED QUIET)
    endif()
  endif()

  find_path(RapidJSON_INCLUDE_DIR NAMES rapidjson/rapidjson.h
                                  PATHS ${PC_RapidJSON_INCLUDEDIR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
                                  REQUIRED_VARS RapidJSON_INCLUDE_DIR RapidJSON_VERSION
                                  VERSION_VAR RapidJSON_VERSION)

if(RAPIDJSON_FOUND)
  set(RAPIDJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
endif()

mark_as_advanced(RapidJSON_INCLUDE_DIR)
