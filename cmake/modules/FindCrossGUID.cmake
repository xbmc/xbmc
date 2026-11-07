# FindCrossGUID
# -------
# Finds the CrossGUID library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::CrossGUID   - The CrossGUID library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroCrossGUID)

    set(CROSSGUID_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX "-dgb")

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_NEW_CROSSGUID)

    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/001-fix-unused-function.patch"
                "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/002-disable-Wall-error.patch"
                "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/003-add-cstdint-include.patch"
                "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/004-cmake-install.patch")

    generate_patchcommand("${patches}")
    unset(patches)

    set(CMAKE_ARGS -DCROSSGUID_TESTS=OFF
                   -DDISABLE_WALL=ON)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Additional pkg-config search name
  list(APPEND ${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC crossguid2)

  # Android builds use Kodi's own CrossGUID dependency build. Do not accept
  # a cached CMake or pkg-config package from the shared dependency prefix:
  # its metadata can outlive the library and leave the final link with -lcrossguid
  # but no corresponding library. Other platforms retain normal package discovery.
  if(ANDROID)
    message(STATUS "Building CrossGUID internally for Android")
    set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND FALSE)
  else()
    SEARCH_EXISTING_PACKAGES()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_CROSSGUID)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

      get_target_property(_crossguid_include_dir PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)

      # NEW_CROSSGUID >= 0.2.0 release
      if(EXISTS "${_crossguid_include_dir}/crossguid/guid.hpp")
        list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_NEW_CROSSGUID)
      endif()
    elseif(TARGET crossguid AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS crossguid)

      get_target_property(_crossguid_include_dir crossguid INTERFACE_INCLUDE_DIRECTORIES)

      # NEW_CROSSGUID >= 0.2.0 release
      if(EXISTS "${_crossguid_include_dir}/crossguid/guid.hpp")
        list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_NEW_CROSSGUID)
      endif()
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    if(UNIX AND NOT (APPLE OR ANDROID))
      # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
      set(FPHSA_NAME_MISMATCHED 1)
      find_package(UUID REQUIRED ${SEARCH_QUIET})
      unset(FPHSA_NAME_MISMATCHED)

      get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)
      if(_ALIASTARGET)
        set(LIB_TARGET ${_ALIASTARGET})
      else()
        set(LIB_TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
      endif()

      if(TARGET UUID::UUID)
        target_link_libraries(${LIB_TARGET} INTERFACE UUID::UUID)
      endif()
    endif()

    # The Android overload of CrossGUID::newGuid requires GUID_ANDROID,
    # regardless of whether CrossGUID is built internally or found externally.
    if(ANDROID)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS GUID_ANDROID)
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(CrossGUID_FIND_REQUIRED)
      message(FATAL_ERROR "CrossGUID libraries were not found. You may want to use -DENABLE_INTERNAL_CROSSGUID=ON")
    endif()
  endif()
endif()
