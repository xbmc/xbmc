#.rst:
# FindSqlite3
# -----------
# Finds the SQLite3 library
#
# This will define the following target:
#
#   SQLite3::SQLite3 - The SQLite3 library
#

if(NOT TARGET SQLite3::SQLite3)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SQLITE3 sqlite3 QUIET)
  endif()

  find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h
                                HINTS ${PC_SQLITE3_INCLUDEDIR}
                                NO_CACHE)
  find_library(SQLITE3_LIBRARY NAMES sqlite3
                               HINTS ${PC_SQLITE3_LIBDIR}
                               NO_CACHE)

  set(SQLITE3_VERSION ${PC_SQLITE3_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Sqlite3
                                    REQUIRED_VARS SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR
                                    VERSION_VAR SQLITE3_VERSION)

  if(Sqlite3_FOUND)
    add_library(SQLite3::SQLite3 UNKNOWN IMPORTED)
    set_target_properties(SQLite3::SQLite3 PROPERTIES
                                           IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}")

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP SQLite3::SQLite3)
  endif()
endif()
