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
    set(LIBDATACHANNEL_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Also-use-static-library-name-in-find_library.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Only-install-static-library.patch")

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
                                    RTC_SYSTEM_SRTP=1
                                    ${EXTRA_DEFINITIONS})

    BUILD_DEP_TARGET()

    add_dependencies(libdatachannel ${APP_NAME_LC}::Libjuice
                                    ${APP_NAME_LC}::Libsrtp
                                    ${APP_NAME_LC}::Plog
                                    ${APP_NAME_LC}::Usrsctp)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # TODO: Check dependencies if we have any intention to build internal
  if (ENABLE_INTERNAL_LIBDATACHANNEL)
    find_package(Libjuice REQUIRED)
    find_package(Libsrtp REQUIRED)
    find_package(Plog REQUIRED)
    find_package(Usrsctp REQUIRED)
  endif()

  set(MODULE_LC libdatachannel)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libdatachannel. If version >= LIBDATACHANNEL-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    # Build lib
    buildLibdatachannel()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBDATACHANNEL)
  unset(LIBDATACHANNEL_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libdatachannel
                                    REQUIRED_VARS LIBDATACHANNEL_LIBRARY LIBDATACHANNEL_INCLUDE_DIR
                                    VERSION_VAR LIBDATACHANNEL_VERSION)

  if(Libdatachannel_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(LIBDATACHANNEL_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${LIBDATACHANNEL_LIBRARY_RELEASE}")
    endif()
    if(LIBDATACHANNEL_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${LIBDATACHANNEL_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::Libjuice;${APP_NAME_LC}::Libsrtp;${APP_NAME_LC}::Usrsctp"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBDATACHANNEL_INCLUDE_DIR}")

    if(_libdatachannel_definitions)
      # We need to append in case the cmake config already has definitions
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS "${_libdatachannel_definitions}")
    endif()

    if(TARGET libdatachannel)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libdatachannel)
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
      if(NOT TARGET libdatachannel)
        buildLibdatachannel()
        set_target_properties(libdatachannel PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libdatachannel)
    endif()
  else()
    if(Libdatachannel_FIND_REQUIRED)
      message(FATAL_ERROR "libdatachannel libraries were not found. You may want to try -DENABLE_INTERNAL_LIBDATACHANNEL=ON")
    endif()
  endif()
endif()
