#.rst:
# FindSqlite3
# -----------
# Finds the SQLite3 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Sqlite3 - The SQLite3 library
#   LIBRARY::Sqlite3 - ALIAS TARGET for the SQLite3 library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC sqlite3)

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET sqlite3::sqlite3)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS sqlite3::sqlite3)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS sqlite3::sqlite3)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    endif()
  else()
    if(Sqlite3_FIND_REQUIRED)
      message(FATAL_ERROR "SQLite3 library not found.")
    endif()
  endif()
endif()
