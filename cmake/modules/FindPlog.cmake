#.rst:
# FindPlog
# -------
# Finds the plog library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Plog - The plog library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildPlog)
    set(PLOG_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DPLOG_BUILD_SAMPLES=OFF
                   -DPLOG_INSTALL=ON)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC plog)

  SETUP_BUILD_VARS()

  # TODO: Check for existing plog. If version >= PLOG-VERSION file version, dont build
  if(ENABLE_INTERNAL_PLOG)
    # Build lib
    buildPlog()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(PLOG)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Plog
                                    REQUIRED_VARS PLOG_LIBRARY PLOG_INCLUDE_DIR
                                    VERSION_VAR PLOG_VERSION)

  if(Plog_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${PLOG_INCLUDE_DIR}")

    if(TARGET plog)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} plog)
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
      if(NOT TARGET plog)
        buildPlog()
        set_target_properties(plog PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends plog)
    endif()
  else()
    if(Plog_FIND_REQUIRED)
      message(FATAL_ERROR "plog libraries were not found. You may want to try -DENABLE_INTERNAL_PLOG=ON")
    endif()
  endif()
endif()
