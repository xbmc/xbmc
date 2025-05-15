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

    if(ANDROID)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS GUID_ANDROID)
    endif()

    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/001-fix-unused-function.patch"
                "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/002-disable-Wall-error.patch"
                "${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/003-add-cstdint-include.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCROSSGUID_TESTS=OFF
                   -DDISABLE_WALL=ON)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  if(ENABLE_INTERNAL_CROSSGUID)
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    find_package(PkgConfig ${SEARCH_QUIET})
    # Do not use pkgconfig on windows
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)

      if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
        # First item is the full path of the library file found
        # pkg_check_modules does not populate a variable of the found library explicitly
        list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)

        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
      endif()
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(CrossGUID_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

      # NEW_CROSSGUID >= 0.2.0 release
      if(EXISTS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}/crossguid/guid.hpp")
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

    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(CrossGUID_FIND_REQUIRED)
      message(FATAL_ERROR "CrossGUID libraries were not found. You may want to use -DENABLE_INTERNAL_CROSSGUID=ON")
    endif()
  endif()
endif()
