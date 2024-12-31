#.rst:
# FindHarfbuzz
# ------------
# Finds the HarfBuzz library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::HarfBuzz   - The HarfBuzz library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_HARFBUZZ harfbuzz QUIET)
  endif()

  find_path(HARFBUZZ_INCLUDE_DIR NAMES harfbuzz/hb-ft.h hb-ft.h
                                 HINTS ${DEPENDS_PATH}/include
                                       ${PC_HARFBUZZ_INCLUDEDIR}
                                       ${PC_HARFBUZZ_INCLUDE_DIRS}
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  find_library(HARFBUZZ_LIBRARY NAMES harfbuzz
                                HINTS ${DEPENDS_PATH}/lib ${PC_HARFBUZZ_LIBDIR}
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  set(HARFBUZZ_VERSION ${PC_HARFBUZZ_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(HarfBuzz
                                    REQUIRED_VARS HARFBUZZ_LIBRARY HARFBUZZ_INCLUDE_DIR
                                    VERSION_VAR HARFBUZZ_VERSION)

  if(HARFBUZZ_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${HARFBUZZ_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}")

    if(NOT TARGET harfbuzz::harfbuzz)
      add_library(harfbuzz::harfbuzz ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    endif()
  else()
    if(HarfBuzz_FIND_REQUIRED)
      message(FATAL_ERROR "Harfbuzz libraries were not found.")
    endif()
  endif()
endif()
