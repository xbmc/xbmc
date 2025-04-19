#.rst:
# FindSqlite3
# -----------
# Finds the SQLite3 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Sqlite3 - The SQLite3 library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_SQLITE3 sqlite3 ${SEARCH_QUIET})

    set(SQLITE3_VERSION ${PC_SQLITE3_VERSION})
  endif()

  find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h
                             HINTS ${DEPENDS_PATH}/include ${PC_SQLITE3_INCLUDEDIR}
                             ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  find_library(SQLITE3_LIBRARY NAMES sqlite3
                               HINTS ${DEPENDS_PATH}/lib ${PC_SQLITE3_LIBDIR}
                               ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Sqlite3
                                    REQUIRED_VARS SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR
                                    VERSION_VAR SQLITE3_VERSION)

  if(Sqlite3_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}")
  else()
    if(Sqlite3_FIND_REQUIRED)
      message(FATAL_ERROR "SQLite3 library not found.")
    endif()
  endif()
endif()
