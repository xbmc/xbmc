#.rst:
# FindDate
# -------
# Finds the DATE library and builds internal
# DATE if requested
#
# This will define the following variables::
#
# DATE_FOUND - system has DATE
# DATE_INCLUDE_DIRS - the DATE include directory
# DATE_LIBRARIES - the DATE libraries
# DATE_DEFINITIONS - the DATE definitions
#
# and the following imported targets::
#
#   Date::Date   - The Date library

if(ENABLE_INTERNAL_DATE)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC date)

  SETUP_BUILD_VARS()

  set(DATE_VERSION ${${MODULE}_VER})

  # Debug postfix only used for windows
  if(WIN32 OR WINDOWS_STORE)
    set(DATE_DEBUG_POSTFIX "d")
  endif()

  # Propagate CMake definitions

  if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    set(EXTRA_ARGS -DIOS=ON)
  elseif(WINDOWS_STORE)
    set(EXTRA_ARGS -DWINRT=ON)
  endif()

  set(CMAKE_ARGS -DCMAKE_CXX_STANDARD=17
                 -DUSE_SYSTEM_TZ_DB=OFF
                 -DMANUAL_TZ_DB=ON
                 -DUSE_TZ_DB_IN_DOT=OFF
                 -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_TZ_LIB=ON
                 ${EXTRA_ARGS})

  # Work around old release

  file(GLOB patches "${CMAKE_SOURCE_DIR}/tools/depends/target/date/*.patch")

  generate_patchcommand("${patches}")

  BUILD_DEP_TARGET()
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_DATE libdate-tz>=3.0.1 QUIET)
  endif()

  find_path(DATE_INCLUDE_DIR date/date.h
                             PATHS ${PC_DATE_INCLUDEDIR})
  find_library(DATE_LIBRARY_RELEASE NAMES date-tz libdate-tz
                                    PATHS ${PC_DATE_LIBDIR})
  find_library(DATE_LIBRARY_DEBUG NAMES date-tzd libdate-tzd
                                    PATHS ${PC_DATE_LIBDIR})
  set(DATE_VERSION ${PC_DATE_VERSION})
endif()

include(SelectLibraryConfigurations)
select_library_configurations(DATE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Date
                                  REQUIRED_VARS DATE_LIBRARY DATE_INCLUDE_DIR
                                  VERSION_VAR DATE_VERSION)

if(DATE_FOUND)
  set(DATE_INCLUDE_DIRS ${DATE_INCLUDE_DIR})
  set(DATE_LIBRARIES ${DATE_LIBRARY})

  if(ENABLE_INTERNAL_TZDATA)
    set(DATE_DEFINITIONS ${DATE_DEFINITIONS} -DDATE_INTERNAL_TZDATA)
  endif()

  if(DATE_HAS_STRINGVIEW)
    set(DATE_DEFINITIONS ${DATE_DEFINITIONS} -DDATE_HAS_STRINGVIEW)
  endif()

  if(NOT TARGET Date::Date)
    add_library(Date::Date UNKNOWN IMPORTED)
    if(DATE_LIBRARY_RELEASE)
      set_target_properties(Date::Date PROPERTIES
                                           IMPORTED_CONFIGURATIONS RELEASE
                                           IMPORTED_LOCATION "${DATE_LIBRARY_RELEASE}")
    endif()
    if(DATE_LIBRARY_DEBUG)
      set_target_properties(Date::Date PROPERTIES
                                           IMPORTED_CONFIGURATIONS DEBUG
                                           IMPORTED_LOCATION "${DATE_LIBRARY_DEBUG}")
    endif()
    set_target_properties(Date::Date PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIR}")
    if(TARGET date)
      add_dependencies(Date::Date date)
    endif()
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Date::Date)
endif()

mark_as_advanced(DATE_INCLUDE_DIR DATE_LIBRARY)
