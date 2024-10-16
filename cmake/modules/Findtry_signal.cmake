#.rst:
# Findtry_signal
# -------
# Finds the try_signal library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::try_signal - The try_signal library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildtry_signal)
    set(TRY_SIGNAL_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-CMake-Add-install-step.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Use-C-17.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC try_signal)

  SETUP_BUILD_VARS()

  # TODO: Check for existing try_signal. If version >= TRY_SIGNAL-VERSION file version, dont build
  if(ENABLE_INTERNAL_TRY_SIGNAL)
    # Build lib
    buildtry_signal()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TRY_SIGNAL)
  unset(TRY_SIGNAL_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(try_signal
                                    REQUIRED_VARS TRY_SIGNAL_LIBRARY TRY_SIGNAL_INCLUDE_DIR
                                    VERSION_VAR TRY_SIGNAL_VERSION)

  if(try_signal_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(TRY_SIGNAL_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${TRY_SIGNAL_LIBRARY_RELEASE}")
    endif()
    if(TRY_SIGNAL_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${TRY_SIGNAL_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${TRY_SIGNAL_INCLUDE_DIR}")

    if(TARGET try_signal)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} try_signal)
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
      if(NOT TARGET try_signal)
        buildtry_signal()
        set_target_properties(try_signal PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends try_signal)
    endif()
  else()
    if(try_signal_FIND_REQUIRED)
      message(FATAL_ERROR "try_signal libraries were not found. You may want to try -DENABLE_INTERNAL_TRY_SIGNAL=ON")
    endif()
  endif()
endif()
