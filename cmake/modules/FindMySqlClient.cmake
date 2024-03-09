#.rst:
# FindMySqlClient
# ---------------
# Finds the MySqlClient library
#
# This will define the following target:
#
#   MySqlClient::MySqlClient   - The MySqlClient library

if(NOT TARGET MySqlClient::MySqlClient)
  # Don't find system wide installed version on Windows
  if(WIN32)
    set(EXTRA_FIND_ARGS NO_SYSTEM_ENVIRONMENT_PATH)
  endif()

  find_path(MYSQLCLIENT_INCLUDE_DIR NAMES mysql/mysql.h mysql/server/mysql.h
                                    NO_CACHE)
  find_library(MYSQLCLIENT_LIBRARY_RELEASE NAMES mysqlclient libmysql
                                           PATH_SUFFIXES mysql
                                           ${EXTRA_FIND_ARGS}
                                           NO_CACHE)
  find_library(MYSQLCLIENT_LIBRARY_DEBUG NAMES mysqlclient libmysql
                                         PATH_SUFFIXES mysql
                                         ${EXTRA_FIND_ARGS}
                                         NO_CACHE)

  if(MYSQLCLIENT_INCLUDE_DIR AND EXISTS "${MYSQLCLIENT_INCLUDE_DIR}/mysql/mysql_version.h")
    file(STRINGS "${MYSQLCLIENT_INCLUDE_DIR}/mysql/mysql_version.h" mysql_version_str REGEX "^#define[\t ]+LIBMYSQL_VERSION[\t ]+\".*\".*")
    string(REGEX REPLACE "^#define[\t ]+LIBMYSQL_VERSION[\t ]+\"([^\"]+)\".*" "\\1" MYSQLCLIENT_VERSION_STRING "${mysql_version_str}")
    unset(mysql_version_str)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(MYSQLCLIENT)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MySqlClient
                                    REQUIRED_VARS MYSQLCLIENT_LIBRARY MYSQLCLIENT_INCLUDE_DIR
                                    VERSION_VAR MYSQLCLIENT_VERSION_STRING)

  if(MYSQLCLIENT_FOUND)
    add_library(MySqlClient::MySqlClient UNKNOWN IMPORTED)
    if(MYSQLCLIENT_LIBRARY_RELEASE)
      set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                     IMPORTED_CONFIGURATIONS RELEASE
                                                     IMPORTED_LOCATION_RELEASE "${MYSQLCLIENT_LIBRARY_RELEASE}")
    endif()
    if(MYSQLCLIENT_LIBRARY_DEBUG)
      set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                     IMPORTED_CONFIGURATIONS DEBUG
                                                     IMPORTED_LOCATION_DEBUG "${MYSQLCLIENT_LIBRARY_DEBUG}")
    endif()
    set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                   INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCLIENT_INCLUDE_DIR}"
                                                   INTERFACE_COMPILE_DEFINITIONS HAS_MYSQL=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP MySqlClient::MySqlClient)
  endif()
endif()
