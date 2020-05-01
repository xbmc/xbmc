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
  include(${WITH_KODI_DEPENDS}/packages/rapidjson/package.cmake)
  add_depends_for_targets("HOST")

  add_custom_target(rapidjson ALL DEPENDS rapidjson-host)

  set(RapidJSON_LIBRARY ${INSTALL_PREFIX_HOST}/lib/librapidjson.a)
  set(RapidJSON_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include)

  set_target_properties(rapidjson PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(rapidjson
                                    REQUIRED_VARS RapidJSON_LIBRARY RapidJSON_INCLUDE_DIR
                                    VERSION_VAR RJSON_VER)

  set(RapidJSON_LIBRARIES ${RapidJSON_LIBRARY})
  set(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
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


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
                                  REQUIRED_VARS RapidJSON_INCLUDE_DIR RapidJSON_VERSION
                                  VERSION_VAR RapidJSON_VERSION)

if(RAPIDJSON_FOUND)
  set(RAPIDJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
endif()

mark_as_advanced(RapidJSON_INCLUDE_DIR)

endif()
