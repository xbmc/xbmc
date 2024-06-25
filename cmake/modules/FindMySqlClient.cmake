#.rst:
# FindMySqlClient
# ---------------
# Finds the MySqlClient library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::MySqlClient   - The MySqlClient library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_path(MYSQLCLIENT_INCLUDE_DIR NAMES mysql/mysql.h mysql/server/mysql.h)
  find_library(MYSQLCLIENT_LIBRARY NAMES mysqlclient libmysql
                                   PATH_SUFFIXES mysql
                                   ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  if(MYSQLCLIENT_INCLUDE_DIR AND EXISTS "${MYSQLCLIENT_INCLUDE_DIR}/mysql/mysql_version.h")
    file(STRINGS "${MYSQLCLIENT_INCLUDE_DIR}/mysql/mysql_version.h" mysql_version_str REGEX "^#define[\t ]+LIBMYSQL_VERSION[\t ]+\".*\".*")
    string(REGEX REPLACE "^#define[\t ]+LIBMYSQL_VERSION[\t ]+\"([^\"]+)\".*" "\\1" MYSQLCLIENT_VERSION_STRING "${mysql_version_str}")
    unset(mysql_version_str)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MySqlClient
                                    REQUIRED_VARS MYSQLCLIENT_LIBRARY MYSQLCLIENT_INCLUDE_DIR
                                    VERSION_VAR MYSQLCLIENT_VERSION_STRING)

  if(MYSQLCLIENT_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${MYSQLCLIENT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCLIENT_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAS_MYSQL)
  endif()
endif()
