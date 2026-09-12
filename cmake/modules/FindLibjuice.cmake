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
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-CMake-Only-install-static-library.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DNO_TESTS=ON)

    # Set definitions that will be set in the built cmake config file
    # We dont import the config file if we build internal (chicken/egg scenario)
    set(_libjuice_definitions JUICE_STATIC)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libjuice)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libjuice. If version >= LIBJUICE-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBJUICE)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Build lib
      buildLibjuice()
    endif()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libjuice
                                    REQUIRED_VARS
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Libjuice_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE}")
    endif()
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG}")
    endif()

    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")

    if(_libjuice_definitions)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_COMPILE_DEFINITIONS "${_libjuice_definitions}")
    endif()

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
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
      if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
        buildLibjuice()
        set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Libjuice_FIND_REQUIRED)
      message(FATAL_ERROR "libjuice libraries were not found. You may want to try -DENABLE_INTERNAL_LIBJUICE=ON")
    endif()
  endif()
endif()
