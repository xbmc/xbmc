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
  include(ExternalProject)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  get_archive_name(rapidjson)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(RapidJSON_URL)
    get_filename_component(RapidJSON_URL "${RapidJSON_URL}" ABSOLUTE)
  else()
    set(RapidJSON_URL http://mirrors.kodi.tv/build-deps/sources/${ARCHIVE})
  endif()
  if(VERBOSE)
    message(STATUS "RapidJSON_URL: ${RapidJSON_URL}")
  endif()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(RapidJSON_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/librapidjson.a)
  set(RapidJSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  externalproject_add(rapidjson
                      URL ${RapidJSON_URL}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      PREFIX ${CORE_BUILD_DIR}/rapidjson
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 -DRAPIDJSON_BUILD_DOC=OFF
                                 -DRAPIDJSON_BUILD_EXAMPLES=OFF
                                 -DRAPIDJSON_BUILD_TESTS=OFF
                                 -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF
                                 "${EXTRA_ARGS}"
                      PATCH_COMMAND patch -p1 < ${CORE_SOURCE_DIR}/tools/depends/target/rapidjson/0001-remove_custom_cxx_flags.patch
                      BUILD_BYPRODUCTS ${RapidJSON_LIBRARY})
  set_target_properties(rapidjson PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(rapidjson
                                    REQUIRED_VARS RapidJSON_LIBRARY RapidJSON_INCLUDE_DIR
                                    VERSION_VAR RAPIDJSON_VER)

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
