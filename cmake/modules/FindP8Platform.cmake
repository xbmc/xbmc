# FindP8Platform
# -------
# Finds the P8-Platform library
#
# This will define the following target:
#
#   LIBRARY::P8Platform   - ALIAS target for P8-Platform library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildlibp8platform)
    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-all-fix-c++17-support.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/002-all-fixcmakeinstall.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/003-all-cmake_tweakversion.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/004-all-fix-cxx-standard.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF)

    # p8-platform hasnt tagged a release since 2016. Force this for cmake 4.0
    # p8-platform master has updated cmake minimum to 3.12, so if they ever tag a new release
    # this can be dropped.
    list(APPEND CMAKE_ARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.10)

    # CMAKE_INSTALL_LIBDIR in p8-platform prepends project prefix, so disable sending any
    # install_libdir to generator
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_LIBDIR "/lib")

    set(BUILD_NAME build-p8-platform)

    BUILD_DEP_TARGET()

    set(P8-PLATFORM_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC p8-platform)
  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Search cmake-config. Suitable all platforms
  find_package(p8-platform ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                           HINTS ${DEPENDS_PATH}/lib/cmake
                           ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  # fallback to pkgconfig for non windows platforms
  if(NOT p8-platform_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(p8-platform p8-platform${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  if(p8-platform_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    # build p8-platform lib
    buildlibp8platform()
  else()
    if(TARGET PkgConfig::p8-platform)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET p8-platform_LINK_LIBRARIES 0 P8-PLATFORM_LIBRARY)

      get_target_property(P8-PLATFORM_INCLUDE_DIR PkgConfig::p8-platform INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET p8-platform)

      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET p8-platform_LIBRARIES 0 P8-PLATFORM_LIBRARY)

      set(P8-PLATFORM_INCLUDE_DIR ${p8-platform_INCLUDE_DIRS})
      set(P8-PLATFORM_VERSION ${p8-platform_VERSION})
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(P8Platform
                                    REQUIRED_VARS P8-PLATFORM_LIBRARY P8-PLATFORM_INCLUDE_DIR
                                    VERSION_VAR P8-PLATFORM_VERSION)

  if(P8PLATFORM_FOUND)
    if(TARGET PkgConfig::p8-platform AND NOT TARGET build-p8-platform)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::p8-platform)
    else()
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                IMPORTED_LOCATION "${P8-PLATFORM_LIBRARY}"
                                                                INTERFACE_INCLUDE_DIRECTORIES "${P8-PLATFORM_INCLUDE_DIR}")
  
      if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                  INTERFACE_LINK_LIBRARIES "-framework CoreVideo")
      endif()
    endif()

    if(TARGET build-p8-platform)
      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} build-p8-platform)

      # If the build target exists here, set LIB_BUILD property to allow calling modules
      # know that this will be rebuilt, and they will need to rebuild as well
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
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
      if(NOT TARGET build-p8-platform)
        buildlibp8platform()
        set_target_properties(build-p8-platform PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends build-p8-platform)
    endif()
  else()
    if(P8Platform_FIND_REQUIRED)
      message(FATAL_ERROR "P8-PLATFORM not found.")
    endif()
  endif()
endif()
