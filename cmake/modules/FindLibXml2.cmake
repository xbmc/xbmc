#.rst:
# FindLibXml2
# -----------
# Finds the LibXml2 library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::LibXml2 - The LibXml2 library
#   LIBRARY::LibXml2   - ALIAS TARGET for the LibXml2 library
#   LibXml2::LibXml2   - ALIAS TARGET for the LibXml2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroLibXml2)
    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS "/MP")

      if(WINDOWS_STORE)
        string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS " /D_CRT_SECURE_NO_WARNINGS /sdl-")
      endif()

      generate_patchcommand("${CMAKE_SOURCE_DIR}/tools/depends/target/libxml2/01-win-pkgconfig-libname.patch")
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${DEPENDS_PATH}/include/libxml2)
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})

    set(CMAKE_ARGS -DLIBXML2_WITH_DEBUG=OFF
                   -DLIBXML2_WITH_ICONV=OFF
                   -DLIBXML2_WITH_MODULES=OFF
                   -DLIBXML2_WITH_PYTHON=OFF
                   -DLIBXML2_WITH_PROGRAMS=OFF
                   -DLIBXML2_WITH_TESTS=OFF)

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
      list(APPEND CMAKE_ARGS -DBUILD_SHARED_LIBS=ON)
    else()
      list(APPEND CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF)
    endif()

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libxml2)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC libxml-2.0)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_LIBXML2) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_LIBXML2))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET LibXml2::LibXml2 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS LibXml2::LibXml2)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)
    if(NOT _ALIASTARGET)
      set(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    endif()

    # Add ALIAS TARGET as LibXml2 can be a dependency of other libs
    add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_ALIASTARGET})

    # Common TARGET name other libs use
    if(NOT TARGET LibXml2::LibXml2)
      add_library(LibXml2::LibXml2 ALIAS ${_ALIASTARGET})
    endif()
    unset(_ALIASTARGET)

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(LibXml2_FIND_REQUIRED)
      message(FATAL_ERROR "LibXml2 library was not found. You may want to try -DENABLE_INTERNAL_LIBXML2=ON")
    endif()
  endif()
endif()
