#.rst:
# FindMySqlClient
# ---------------
# Finds the MySqlClient library
#
# This will will define the following variables::
#
# MYSQLCLIENT_FOUND - system has MySqlClient
# MYSQLCLIENT_INCLUDE_DIRS - the MySqlClient include directory
# MYSQLCLIENT_LIBRARIES - the MySqlClient libraries
# MYSQLCLIENT_DEFINITIONS - the MySqlClient compile definitions
#
# and the following imported targets::
#
#   MySqlClient::MySqlClient   - The MySqlClient library

# Don't find system wide installed version on Windows
if(WIN32)
  set(EXTRA_FIND_ARGS NO_SYSTEM_ENVIRONMENT_PATH)
else()
  set(EXTRA_FIND_ARGS)
endif()

find_path(MYSQLCLIENT_INCLUDE_DIR mysql/mysql_time.h)
find_library(MYSQLCLIENT_LIBRARY_RELEASE NAMES mysqlclient libmysql
                                         PATH_SUFFIXES mysql
                                         ${EXTRA_FIND_ARGS})
find_library(MYSQLCLIENT_LIBRARY_DEBUG NAMES mysqlclient libmysql
                                       PATH_SUFFIXES mysql
                                       ${EXTRA_FIND_ARGS})

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
  set(MYSQLCLIENT_LIBRARIES ${MYSQLCLIENT_LIBRARY})
  set(MYSQLCLIENT_INCLUDE_DIRS ${MYSQLCLIENT_INCLUDE_DIR})
  set(MYSQLCLIENT_DEFINITIONS -DHAVE_MYSQL=1)

  if(NOT TARGET MySqlClient::MySqlClient)
    add_library(MySqlClient::MySqlClient UNKNOWN IMPORTED)
    if(MYSQLCLIENT_LIBRARY_RELEASE)
      set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                     IMPORTED_CONFIGURATIONS RELEASE
                                                     IMPORTED_LOCATION "${MYSQLCLIENT_LIBRARY_RELEASE}")
    endif()
    if(MYSQLCLIENT_LIBRARY_DEBUG)
      set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                     IMPORTED_CONFIGURATIONS DEBUG
                                                     IMPORTED_LOCATION "${MYSQLCLIENT_LIBRARY_DEBUG}")
    endif()
    set_target_properties(MySqlClient::MySqlClient PROPERTIES
                                                   INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCLIENT_INCLUDE_DIR}"
                                                   INTERFACE_COMPILE_DEFINITIONS HAVE_MYSQL=1)
  endif()
endif()

mark_as_advanced(MYSQLCLIENT_INCLUDE_DIR MYSQLCLIENT_LIBRARY)
