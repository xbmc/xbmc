#.rst:
# FindLibdatachannel
# -------
# Finds the libdatachannel library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Libdatachannel - The libdatachannel library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibdatachannel)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-Also-use-static-library-name-in-find_library.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0002-Only-install-static-library.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -DNO_EXAMPLES=ON
                   -DNO_TESTS=ON
                   -DPREFER_SYSTEM_LIB=ON)

    # Set definitions that will be set in the built cmake config file
    # We dont import the config file if we build internal (chicken/egg scenario)
    set(_libdatachannel_definitions RTC_STATIC
                                    RTC_SYSTEM_JUICE=1
                                    RTC_SYSTEM_SRTP=1)

    BUILD_DEP_TARGET()

    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}
                     ${APP_NAME_LC}::Libjuice
                     ${APP_NAME_LC}::Libsrtp
                     ${APP_NAME_LC}::Plog
                     ${APP_NAME_LC}::Usrsctp)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # TODO: Check dependencies if we have any intention to build internal
  if (ENABLE_INTERNAL_LIBDATACHANNEL)
    find_package(Libjuice REQUIRED ${SEARCH_QUIET})
    find_package(Libsrtp REQUIRED ${SEARCH_QUIET})
    find_package(Plog REQUIRED ${SEARCH_QUIET})
    find_package(Usrsctp REQUIRED ${SEARCH_QUIET})
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdatachannel)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libdatachannel. If version >= LIBDATACHANNEL-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Build lib
      buildLibdatachannel()
    endif()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libdatachannel
                                    REQUIRED_VARS
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Libdatachannel_FOUND)
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
    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::Libjuice;${APP_NAME_LC}::Libsrtp;${APP_NAME_LC}::Usrsctp")

    if(_libdatachannel_definitions)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS "${_libdatachannel_definitions}")
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
        buildLibdatachannel()
        set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Libdatachannel_FIND_REQUIRED)
      message(FATAL_ERROR "libdatachannel libraries were not found. You may want to try -DENABLE_INTERNAL_LIBDATACHANNEL=ON")
    endif()
  endif()
endif()
