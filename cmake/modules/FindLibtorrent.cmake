#.rst:
# FindLibtorrent
# -------
# Finds the libtorrent library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Libtorrent - The libtorrent library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibtorrent)
    set(LIBTORRENT_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-Use-system-libdatachannel-and-try_signal.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0002-Disable-asserts.patch")

    if(WINDOWS_STORE)
      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0003-Disable-Win32-file-API-in-Boost.patch")
    endif()

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -Ddeprecated-functions=OFF
                   -Dwebtorrent=ON)

    # Set definitions that will be set in the built cmake config file
    # We dont import the config file if we build internal (chicken/egg scenario)
    set(_libtorrent_definitions TORRENT_ABI_VERSION=4 # libtorrent-2.1
                                TORRENT_USE_LIBCRYPTO
                                TORRENT_USE_OPENSSL)

    BUILD_DEP_TARGET()

    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${APP_NAME_LC}::Boost
                                  ${APP_NAME_LC}::Libdatachannel
                                  ${APP_NAME_LC}::try_signal)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # TODO: Check dependencies if we have any intention to build internal
  if (ENABLE_INTERNAL_LIBTORRENT)
    find_package(Boost REQUIRED)
    find_package(Libdatachannel REQUIRED)
    find_package(try_signal REQUIRED)
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libtorrent)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libtorrent. If version >= LIBTORRENT-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBTORRENT)
    # Build lib
    buildLibtorrent()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBTORRENT)
  unset(LIBTORRENT_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libtorrent
                                    REQUIRED_VARS LIBTORRENT_LIBRARY LIBTORRENT_INCLUDE_DIR
                                    VERSION_VAR LIBTORRENT_VERSION)

  if(Libtorrent_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    if(LIBTORRENT_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${LIBTORRENT_LIBRARY_RELEASE}")
    endif()
    if(LIBTORRENT_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${LIBTORRENT_LIBRARY_DEBUG}")
    endif()

    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_INCLUDE_DIRECTORIES "${LIBTORRENT_INCLUDE_DIR}")
    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_COMPILE_DEFINITIONS "HAS_LIBTORRENT")
    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::Boost;${APP_NAME_LC}::Libdatachannel;${APP_NAME_LC}::try_signal")

    if(_libtorrent_definitions)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_COMPILE_DEFINITIONS "${_libtorrent_definitions}")
    endif()

    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_LINK_LIBRARIES "-framework SystemConfiguration")
    endif()

    if(TARGET libtorrent)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libtorrent)
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
      if(NOT TARGET libtorrent)
        buildLibtorrent()
        set_target_properties(libtorrent PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libtorrent)
    endif()
  else()
    if(Libtorrent_FIND_REQUIRED)
      message(FATAL_ERROR "libtorrent libraries were not found. You may want to try -DENABLE_INTERNAL_LIBTORRENT=ON")
    endif()
  endif()
endif()
