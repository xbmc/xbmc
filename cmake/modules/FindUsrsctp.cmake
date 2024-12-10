#.rst:
# FindUsrsctp
# -------
# Finds the usrsctp library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Usrsctp - The usrsctp library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildUsrsctp)
    set(USRSCTP_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Add-CMake-export-targets.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -Dsctp_build_programs=OFF
                   -Dsctp_werror=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC usrsctp)

  SETUP_BUILD_VARS()

  # TODO: Check for existing usrsctp. If version >= USRSCTP-VERSION file version, dont build
  if(ENABLE_INTERNAL_USRSCTP)
    # Build lib
    buildUsrsctp()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(USRSCTP)
  unset(USRSCTP_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Usrsctp
                                    REQUIRED_VARS USRSCTP_LIBRARY USRSCTP_INCLUDE_DIR
                                    VERSION_VAR USRSCTP_VERSION)

  if(Usrsctp_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(USRSCTP_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${USRSCTP_LIBRARY_RELEASE}")
    endif()
    if(USRSCTP_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${USRSCTP_LIBRARY_DEBUG}")
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${USRSCTP_INCLUDE_DIR}")

    if(TARGET usrsctp)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} usrsctp)
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
      if(NOT TARGET usrsctp)
        buildUsrsctp()
        set_target_properties(usrsctp PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends usrsctp)
    endif()
  else()
    if(Usrsctp_FIND_REQUIRED)
      message(FATAL_ERROR "usrsctp libraries were not found. You may want to try -DENABLE_INTERNAL_USRSCTP=ON")
    endif()
  endif()
endif()
