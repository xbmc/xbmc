# FindP8Platform
# -------
# Finds the P8-Platform library
#
# This will define the following target:
#
#   LIBRARY::P8Platform   - ALIAS target for P8-Platform library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroP8Platform)
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

    BUILD_DEP_TARGET()

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework CoreVideo")
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC p8-platform)
  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    # build p8-platform lib
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET p8-platform)
      # p8platform cmake config is not target ready. specifically use variable names
      # First item is the full path of the library file found
      list(GET p8-platform_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY)

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR p8-platform_INCLUDE_DIRS})
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION p8-platform_VERSION})
    endif()
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(P8Platform
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(P8Platform_FOUND)
    if(TARGET p8-platform AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS p8-platform)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # If the build target exists here, set LIB_BUILD property to allow calling modules
      # know that this will be rebuilt, and they will need to rebuild as well
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(P8Platform_FIND_REQUIRED)
      message(FATAL_ERROR "P8-PLATFORM not found.")
    endif()
  endif()
endif()
