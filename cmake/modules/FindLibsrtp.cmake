#.rst:
# FindLibsrtp
# -------
# Finds the libsrtp library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Libsrtp - The libsrtp library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibsrtp)
    set(LIBSRTP_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DBUILD_WITH_WARNINGS=OFF
                   -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DENABLE_OPENSSL=ON
                   -DLIBSRTP_TEST_APPS=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libsrtp)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libsrtp. If version >= LIBSRTP-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBSRTP)
    # Build lib
    buildLibsrtp()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBSRTP)
  unset(LIBSRTP_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libsrtp
                                    REQUIRED_VARS LIBSRTP_LIBRARY LIBSRTP_INCLUDE_DIR
                                    VERSION_VAR LIBSRTP_VERSION)

  if(Libsrtp_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(LIBSRTP_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${LIBSRTP_LIBRARY_RELEASE}")
    endif()
    if(LIBSRTP_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${LIBSRTP_LIBRARY_DEBUG}")
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBSRTP_INCLUDE_DIR}")

    if(TARGET libsrtp)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libsrtp)
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
      if(NOT TARGET libsrtp)
        buildLibsrtp()
        set_target_properties(libsrtp PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libsrtp)
    endif()
  else()
    if(Libsrtp_FIND_REQUIRED)
      message(FATAL_ERROR "libsrtp libraries were not found. You may want to try -DENABLE_INTERNAL_LIBSRTP=ON")
    endif()
  endif()
endif()
