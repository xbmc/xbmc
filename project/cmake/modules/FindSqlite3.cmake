# - Try to find SQLITE3
# Once done this will define
#
# SQLITE3_FOUND - system has sqlite3
# SQLITE3_INCLUDE_DIRS - the sqlite3 include directory
# SQLITE3_LIBRARIES - The sqlite3 libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (SQLITE3 sqlite3)
  list(APPEND SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDEDIR})
else()
  find_path(SQLITE3_INCLUDE_DIRS sqlite3.h)
  find_library(SQLITE3_LIBRARIES sqlite3)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sqlite3 DEFAULT_MSG SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)

mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)
