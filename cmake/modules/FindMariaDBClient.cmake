#.rst:
# FindMariaDBClient
# ---------------
# Finds the MariaDBClient library
#
# This will define the following variables::
#
# MARIADBCLIENT_FOUND - system has MariaDBClient
# MARIADBCLIENT_INCLUDE_DIRS - the MariaDBClient include directory
# MARIADBCLIENT_LIBRARIES - the MariaDBClient libraries
# MARIADBCLIENT_DEFINITIONS - the MariaDBClient compile definitions
#
# and the following imported targets::
#
#   MariaDBClient::MariaDBClient   - The MariaDBClient library

# Don't find system wide installed version on Windows
if(WIN32)
  set(EXTRA_FIND_ARGS NO_SYSTEM_ENVIRONMENT_PATH)
else()
  set(EXTRA_FIND_ARGS)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MARIADBCLIENT mariadb QUIET)
endif()


find_path(MARIADBCLIENT_INCLUDE_DIR NAMES mariadb/mysql.h mariadb/server/mysql.h
                                           PATHS ${PC_MARIADBCLIENT_INCLUDEDIR})
find_library(MARIADBCLIENT_LIBRARY_RELEASE NAMES mariadbclient mariadb libmariadb
                                           PATHS ${PC_MARIADBCLIENT_LIBDIR}
                                           PATH_SUFFIXES mariadb
                                           ${EXTRA_FIND_ARGS})
find_library(MARIADBCLIENT_LIBRARY_DEBUG NAMES mariadbclient mariadb libmariadbd
                                         PATHS ${PC_MARIADBCLIENT_LIBDIR}
                                         PATH_SUFFIXES mariadb
                                         ${EXTRA_FIND_ARGS})

if(PC_MARIADBCLIENT_VERSION)
  set(MARIADBCLIENT_VERSION_STRING ${PC_MARIADBCLIENT_VERSION})
elseif(MARIADBCLIENT_INCLUDE_DIR AND EXISTS "${MARIADBCLIENT_INCLUDE_DIR}/mariadb/mariadb_version.h")
  file(STRINGS "${MARIADBCLIENT_INCLUDE_DIR}/mariadb/mariadb_version.h" mariadb_version_str REGEX "^#define[\t ]+MARIADB_CLIENT_VERSION_STR[\t ]+\".*\".*")
  string(REGEX REPLACE "^#define[\t ]+MARIADB_CLIENT_VERSION_STR[\t ]+\"([^\"]+)\".*" "\\1" MARIADBCLIENT_VERSION_STRING "${mariadb_version_str}")
  unset(mariadb_version_str)
endif()

include(SelectLibraryConfigurations)
select_library_configurations(MARIADBCLIENT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MariaDBClient
                                  REQUIRED_VARS MARIADBCLIENT_LIBRARY MARIADBCLIENT_INCLUDE_DIR
                                  VERSION_VAR MARIADBCLIENT_VERSION_STRING)

if(MARIADBCLIENT_FOUND)
  set(MARIADBCLIENT_LIBRARIES ${MARIADBCLIENT_LIBRARY})
  set(MARIADBCLIENT_INCLUDE_DIRS ${MARIADBCLIENT_INCLUDE_DIR})
  set(MARIADBCLIENT_DEFINITIONS -DHAS_MARIADB=1)

  if(CORE_SYSTEM_NAME STREQUAL osx)
    list(APPEND DEPLIBS "-lgssapi_krb5")
  endif()

  if(NOT TARGET MariaDBClient::MariaDBClient)
    add_library(MariaDBClient::MariaDBClient UNKNOWN IMPORTED)
    if(MARIADBCLIENT_LIBRARY_RELEASE)
      set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                         IMPORTED_LOCATION "${MARIADBCLIENT_LIBRARY_RELEASE}")
    endif()
    if(MARIADBCLIENT_LIBRARY_DEBUG)
      set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                         IMPORTED_CONFIGURATIONS DEBUG
                                                         IMPORTED_LOCATION "${MARIADBCLIENT_LIBRARY_DEBUG}")
    endif()
    set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                       INTERFACE_INCLUDE_DIRECTORIES "${MARIADBCLIENT_INCLUDE_DIR}"
                                                       INTERFACE_COMPILE_DEFINITIONS HAS_MARIADB=1)
  endif()
endif()

mark_as_advanced(MARIADBCLIENT_INCLUDE_DIR MARIADBCLIENT_LIBRARY)
