#.rst:
# FindLibAACS
# ----------
# Finds the libaacs library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibAACS   - The libaacs library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildLibAACS)
    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-remove_versioning.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-all-AACS_HOME-env.patch")

    if(CORE_SYSTEM_NAME MATCHES windows)
      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/03-win-cmake.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/04-win-UWP.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/05-win-fix_gcrypt_build.patch")
    endif()

    generate_patchcommand("${patches}")

    if(CORE_SYSTEM_NAME MATCHES windows)
      find_package(libgpg-error CONFIG REQUIRED)
      find_package(libgcrypt CONFIG REQUIRED)
      find_package(iconv CONFIG REQUIRED)

      set(CMAKE_ARGS -DNATIVEPREFIX=${NATIVEPREFIX})
    else()

      if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Free|Net|Open)BSD")
        find_program(MAKE_EXECUTABLE gmake)
      endif()
      find_program(MAKE_EXECUTABLE make REQUIRED)
      find_program(AUTORECONF autoreconf REQUIRED)

      pkg_check_modules(PC_LIBGCRYPT REQUIRED QUIET libgcrypt)
      pkg_check_modules(PC_GPGERROR REQUIRED QUIET gpg-error)

      set(CONFIGURE_COMMAND ./bootstrap
                    COMMAND ./configure
                            --prefix=${DEPENDS_PATH}
                            --disable-static
                            --exec-prefix=${DEPENDS_PATH})

      set(BUILD_COMMAND ${MAKE_EXECUTABLE})
      set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
      set(BUILD_IN_SOURCE 1)

      if(CORE_SYSTEM_NAME STREQUAL "osx")
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT_EXTENSION "dylib")
      endif()
    endif()

    BUILD_DEP_TARGET()

    set(LIBAACS_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libaacs)

  SETUP_BUILD_VARS()

  if(CORE_SYSTEM_NAME MATCHES windows)

    # Kodi libaacs-0.9.0-* prebuilt libs have a broken cmake config file
    # We need to detect this, otherwise the call to find_package will run and error out
    if(EXISTS ${DEPENDS_PATH}/lib/cmake/libaacs/libaacs-config.cmake)
      file(READ ${DEPENDS_PATH}/lib/cmake/libaacs/libaacs-config.cmake aacs_config_output)
      string(FIND ${aacs_config_output} "libbdplus.cmake" BROKEN_CONFIG)
    else()
      set(BROKEN_CONFIG -1)
    endif()

    # Only do a package check if the broken config is not found
    if(NOT (${BROKEN_CONFIG} GREATER "-1"))
      find_package(libaacs CONFIG
                        HINTS ${DEPENDS_PATH}/lib/cmake
                        ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
      set(LIBAACS_VERSION ${libaacs_VERSION})
    endif()
  else()
    find_package(PkgConfig QUIET)

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(LIBAACS libaacs IMPORTED_TARGET GLOBAL QUIET)
    endif()
  endif()

  # Check for existing libaacs. If version >= LIBAACS-VERSION file version, dont build
  if(LIBAACS_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    buildLibAACS()
  else()
    # Our custom cmake target (windows only)
    if(TARGET libaacs::libaacs)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_LIBAACS_CONFIGURATIONS libaacs::libaacs IMPORTED_CONFIGURATIONS)
      foreach(_libaacs_config IN LISTS _LIBAACS_CONFIGURATIONS)
        string(TOUPPER ${_libaacs_config} _libaacs_config_UPPER)
        get_target_property(LIBAACS_LIBRARY_${_libaacs_config_UPPER} libaacs::libaacs IMPORTED_LOCATION_${_libaacs_config_UPPER})
      endforeach()

      get_target_property(LIBAACS_INCLUDE_DIR libaacs::libaacs INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::LIBAACS)
      get_target_property(LIBAACS_LIBRARY_RELEASE PkgConfig::LIBAACS INTERFACE_LINK_LIBRARIES)
      get_target_property(LIBAACS_INCLUDE_DIR PkgConfig::LIBAACS INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBAACS)
  unset(LIBAACS_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibAACS
                                    REQUIRED_VARS LIBAACS_LIBRARY LIBAACS_INCLUDE_DIR
                                    VERSION_VAR LIBAACS_VERSION)

  if(LIBAACS_FOUND)
    # pkgconfig populate target that is sufficient version
    if(TARGET PkgConfig::LIBAACS AND NOT TARGET libaacs)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::LIBAACS)
    # windows cmake config populated target
    elseif(TARGET libaacs::libaacs AND NOT TARGET libaacs)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libaacs::libaacs)
    # otherwise we are building
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${LIBAACS_INCLUDE_DIR}")

      if(LIBAACS_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${LIBAACS_LIBRARY_RELEASE}")
      endif()
      if(LIBAACS_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${LIBAACS_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libaacs)
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
      if(NOT TARGET libaacs)
        buildLibAACS()
        set_target_properties(libaacs PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libaacs)
    endif()
  else()
    if(LibAACS_FIND_REQUIRED)
      message(FATAL_ERROR "libaacs libraries were not found.")
    endif()
  endif()
endif()
