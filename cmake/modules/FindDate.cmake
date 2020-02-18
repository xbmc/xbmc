# FindDate
# -------
# Finds the date library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Date   - The date library

macro(buildDate)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC date)

  SETUP_BUILD_VARS()

  set(DATE_VERSION ${${MODULE}_VER})

  # Debug postfix only used for windows
  if(WIN32 OR WINDOWS_STORE)
    set(DATE_DEBUG_POSTFIX "d")
  endif()

  # Work around old release

  file(GLOB patches "${CMAKE_SOURCE_DIR}/tools/depends/target/date/*.patch")

  generate_patchcommand("${patches}")

  # Propagate CMake definitions

  set(EXTRA_ARGS "")

  if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    list(APPEND EXTRA_ARGS -DIOS=ON)
  endif()

  set(CMAKE_ARGS -DCMAKE_CXX_STANDARD=17
                 -DUSE_SYSTEM_TZ_DB=ON
                 -DMANUAL_TZ_DB=OFF
                 -DUSE_TZ_DB_IN_DOT=OFF
                 -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_TZ_LIB=ON
                 ${EXTRA_ARGS})

  BUILD_DEP_TARGET()
endmacro()

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(ENABLE_INTERNAL_DATE)
    buildDate()
  else()
    find_package(PkgConfig)
    # Do not use pkgconfig on windows
    if(PKG_CONFIG_FOUND AND NOT WIN32)
      pkg_check_modules(PC_DATE date QUIET)
      set(DATE_VERSION ${PC_DATE_VERSION})
    endif()

    find_path(DATE_INCLUDE_DIR NAMES date/date.h date/tz.h
                                    HINTS ${DEPENDS_PATH}/include ${PC_DATE_INCLUDEDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(DATE_LIBRARY_RELEASE NAMES date-tz libdate-tz
                                           HINTS ${DEPENDS_PATH}/lib ${PC_DATE_LIBDIR}
                                           ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(DATE_LIBRARY_DEBUG NAMES date-tzd libdate-tzd
                                         HINTS ${DEPENDS_PATH}/lib ${PC_DATE_LIBDIR}
                                         ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    set(DATE_VERSION ${PC_DATE_VERSION})
  endif()

  # Select relevant lib build type (ie DATE_LIBRARY_RELEASE or DATE_LIBRARY_DEBUG)
  include(SelectLibraryConfigurations)
  select_library_configurations(DATE)
  unset(DATE_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Date
                                    REQUIRED_VARS DATE_LIBRARY DATE_INCLUDE_DIR
                                    VERSION_VAR DATE_VERSION)

  if(DATE_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(DATE_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${DATE_LIBRARY_RELEASE}")
    endif()
    if(DATE_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${DATE_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIRS}")

    if(TARGET date)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} date)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET date)
        buildDate()
        set_target_properties(date PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends date)
    endif()
  else()
    if(Date_FIND_REQUIRED)
      message(FATAL_ERROR "Date libraries were not found. You may want to use -DENABLE_INTERNAL_DATE=ON")
    endif()
  endif()
endif()
