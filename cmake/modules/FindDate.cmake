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

  # Work around old release

  file(GLOB patches "${CMAKE_SOURCE_DIR}/tools/depends/target/date/*.patch")

  generate_patchcommand("${patches}")

  # Propagate CMake definitions

  set(EXTRA_ARGS "")

  # We use date library in header-only mode on Windows
  if(NOT WIN32 AND NOT WINDOWSSTORE)
    list(APPEND EXTRA_ARGS -DBUILD_TZ_LIB=ON)
  endif()

  if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    list(APPEND EXTRA_ARGS -DIOS=ON)
  endif()

  set(CMAKE_ARGS -DCMAKE_CXX_STANDARD=17
                 -DUSE_SYSTEM_TZ_DB=ON
                 -DMANUAL_TZ_DB=OFF
                 -DUSE_TZ_DB_IN_DOT=OFF
                 -DBUILD_SHARED_LIBS=OFF
                 ${EXTRA_ARGS})

  BUILD_DEP_TARGET()
endmacro()

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(ENABLE_INTERNAL_DATE OR WIN32 OR WINDOWSSTORE)
    buildDate()
  else()
    find_package(PkgConfig)

    find_path(DATE_INCLUDE_DIR NAMES date/date.h date/tz.h
                                    HINTS ${DEPENDS_PATH}/include ${PC_DATE_INCLUDEDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(DATE_LIBRARY NAMES date-tz libdate-tz
                                      HINTS ${DEPENDS_PATH}/lib ${PC_DATE_LIBDIR}
                                            ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    set(DATE_VERSION ${PC_DATE_VERSION})
  endif()

  include(FindPackageHandleStandardArgs)

  # We use date library in header-only mode on Windows
  if(NOT WIN32 AND NOT WINDOWSSTORE)
    find_package_handle_standard_args(Date
                                      REQUIRED_VARS DATE_LIBRARY DATE_INCLUDE_DIR
                                      VERSION_VAR DATE_VERSION)
  else()
    find_package_handle_standard_args(Date
                                      REQUIRED_VARS DATE_INCLUDE_DIR
                                      VERSION_VAR DATE_VERSION)
  endif()

  if(DATE_FOUND)
    # We use date library in header-only mode on Windows
    if(NOT WIN32 AND NOT WINDOWSSTORE)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${DATE_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIR}")
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)

      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIR}"
                                                                       INTERFACE_LINK_LIBRARIES "")
    endif()

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
