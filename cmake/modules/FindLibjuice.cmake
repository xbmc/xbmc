#.rst:
# FindLibjuice
# -------
# Finds the libjuice library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Libjuice - The libjuice library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibjuice)
    set(LIBJUICE_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-CMake-Only-install-static-library.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DNO_TESTS=ON)

    # Set definitions that will be set in the built cmake config file
    # We dont import the config file if we build internal (chicken/egg scenario)
    set(_libjuice_definitions JUICE_STATIC
                              ${EXTRA_DEFINITIONS})

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libjuice)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libjuice. If version >= LIBJUICE-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBJUICE)
    # Build lib
    buildLibjuice()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBJUICE)
  unset(LIBJUICE_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libjuice
                                    REQUIRED_VARS LIBJUICE_LIBRARY LIBJUICE_INCLUDE_DIR
                                    VERSION_VAR LIBJUICE_VERSION)

  if(Libjuice_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(LIBJUICE_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${LIBJUICE_LIBRARY_RELEASE}")
    endif()
    if(LIBJUICE_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${LIBJUICE_LIBRARY_DEBUG}")
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBJUICE_INCLUDE_DIR}")

    if(_libjuice_definitions)
      # We need to append in case the cmake config already has definitions
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS "${_libjuice_definitions}")
    endif()

    if(TARGET libjuice)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libjuice)
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
      if(NOT TARGET libjuice)
        buildLibjuice()
        set_target_properties(libjuice PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libjuice)
    endif()
  else()
    if(Libjuice_FIND_REQUIRED)
      message(FATAL_ERROR "libjuice libraries were not found. You may want to try -DENABLE_INTERNAL_LIBJUICE=ON")
    endif()
  endif()
endif()
