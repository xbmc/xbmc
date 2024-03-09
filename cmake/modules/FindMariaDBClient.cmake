#.rst:
# FindMariaDBClient
# ---------------
# Finds the MariaDBClient library
#
# This will define the following target:
#
#   MariaDBClient::MariaDBClient   - The MariaDBClient library

if(NOT TARGET MariaDBClient::MariaDBClient)
  # Don't find system wide installed version on Windows
  if(WIN32)
    set(EXTRA_FIND_ARGS NO_SYSTEM_ENVIRONMENT_PATH)
  endif()

  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_search_module(PC_MARIADBCLIENT libmariadb mariadb QUIET)
  endif()

  find_path(MARIADBCLIENT_INCLUDE_DIR NAMES mariadb/mysql.h mariadb/server/mysql.h
                                      HINTS ${PC_MARIADBCLIENT_INCLUDEDIR}
                                      NO_CACHE)
  find_library(MARIADBCLIENT_LIBRARY_RELEASE NAMES mariadbclient mariadb libmariadb
                                             HINTS ${PC_MARIADBCLIENT_LIBDIR}
                                             PATH_SUFFIXES mariadb
                                             ${EXTRA_FIND_ARGS}
                                             NO_CACHE)
  find_library(MARIADBCLIENT_LIBRARY_DEBUG NAMES mariadbclient mariadb libmariadbd
                                           HINTS ${PC_MARIADBCLIENT_LIBDIR}
                                           PATH_SUFFIXES mariadb
                                           ${EXTRA_FIND_ARGS}
                                           NO_CACHE)

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
    add_library(MariaDBClient::MariaDBClient UNKNOWN IMPORTED)
    if(MARIADBCLIENT_LIBRARY_RELEASE)
      set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                         IMPORTED_LOCATION_RELEASE "${MARIADBCLIENT_LIBRARY_RELEASE}")
    endif()
    if(MARIADBCLIENT_LIBRARY_DEBUG)
      set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                         IMPORTED_CONFIGURATIONS DEBUG
                                                         IMPORTED_LOCATION_DEBUG "${MARIADBCLIENT_LIBRARY_DEBUG}")
    endif()
    set_target_properties(MariaDBClient::MariaDBClient PROPERTIES
                                                       INTERFACE_INCLUDE_DIRECTORIES "${MARIADBCLIENT_INCLUDE_DIR}"
                                                       INTERFACE_COMPILE_DEFINITIONS HAS_MARIADB=1)
    if(CORE_SYSTEM_NAME STREQUAL osx)
      target_link_libraries(MariaDBClient::MariaDBClient INTERFACE gssapi_krb5)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP MariaDBClient::MariaDBClient)
  endif()
endif()
